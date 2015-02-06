#define HOST_LINUX

#include <string.h>

#include <stdio.h> 
#include <asim/syntax.h> 
#include <asim/dralServer.h>
#include "instruction.h"
#include "data_memory.h"
#include "fifo.h"

#define MAX_CYCLES 25
#define IQUEUE_SIZE 4

int main(){

    // ******** Performance Model Declaration / Initialization
    int cycle = 0; //current cycle



    // ******** DRAL-Dreams Declaration / Initiatization ********
    //instantiate a dral server to generate files
    //string is name of output file
    auto server = new DRAL_SERVER_CLASS("dral_trace");

    //Enable events to be written/captured by the server
    server->TurnOn(); 
 
    // Create the nodes of the pipeline
    auto fetch = server->NewNode("FETCH", 0);
    auto schedule = server->NewNode("SCHEDULE", 0);
    auto execute = server->NewNode("EXECUTE", 1);
    auto commit = server->NewNode("COMMIT", 2);
    auto done = server->NewNode("DONE", 3);

    // Create the edges of the pipeline
    auto fetch2schedule = server->NewEdge(fetch, schedule, 1, 1, "FETCH");
    auto schedule2execute = server->NewEdge(schedule, execute, 1, 1, "SCHEDULE");
    auto execute2commit = server->NewEdge(execute, commit, 1, 1, "EXECUTE");
    auto commit2done = server->NewEdge(commit, done, 1, 1, "COMMIT");


    // ******** CPU State Declaration / Initialization ********
    int PC = 0;             //instruction pointer for the CPU

    // Pipeline stage latches
    INSTRUCTION_TOKEN fetch_inst = NULL;
    INSTRUCTION_TOKEN schedule_inst = NULL;
    INSTRUCTION_TOKEN execute_inst = NULL;
    INSTRUCTION_TOKEN commit_inst = NULL;
    INSTRUCTION_TOKEN done_inst = NULL;

    //Instruction memory, each string is ASM instruction
    //this is the program we are wanting to run
    int program_size = 12;
    string instruction_memory[] = {
                       "IMM R0, 128", // base addr 
                       "IMM R1, 1", // stride
                       "IMM R2, 2", // inner loop iterations
                       "IMM R3, 1", // increment
                       "IMM R4, 0", // counter                       
                       "LD R5, R0",
                       "ADD R5, R5, R3", //fetch and increment
                       "ST R5, R0",
                       "ADD R0, R0, R1", //inc address
                       "ADD R4, R4, R3", //inc address
                       "BNE R4, R2, I5",
                       "JMP I4"};

    //Data Memory
    auto data_memory = new DATA_MEMORY_CLASS(1024);// mem size (in 32-bit words) 
     
    //Register File
    auto regfile = new int[8]; // rf size is 8 entries

    //Scoreboard
    int scoreboard; //holds the dest reg of the current executing instruction

    //ibuffer
    auto instruction_queue = new  FIFO_CLASS<INSTRUCTION_TOKEN>(IQUEUE_SIZE);

    //signal to flush frontend of pipeline on branch taken
    bool frontend_flush;


    //Embed the ADF
    //- write the xml into a char string
    //- create comment with "magic number" 0xadf to identify as adf file
    FILE *f = fopen("test4_adf.xml", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* string = new char[fsize + 1];
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0; //null terminate
    server->Comment (0x0adf, string, true);


    // ******** Performance Model Execution ********
    //"clock" our system using a for loop.
    server->StartActivity(0);
    for (cycle= 0; cycle < MAX_CYCLES; cycle++){

        //Reset the control signals
        scoreboard = -1;
        frontend_flush = false;

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);
	// Stupid systolic pipeline (so I can cheat on passing instructions around)

        // Handle instruction end of life

        done_inst = commit_inst;
        if (done_inst != NULL) {
	        server->DeleteItem(done_inst->GetDralItem());
            delete done_inst;
	    }

	// Handle commit of instruction

        commit_inst = execute_inst;
        if (commit_inst != NULL) {
	        server->MoveItem(commit2done, commit_inst->GetDralItem());
	    }

	// Handle execute of instruction

        execute_inst = schedule_inst;
        if (execute_inst != NULL) { 
            int address;
            scoreboard = execute_inst->reg_dst; //if not dstreg, -1

            switch (execute_inst->op){
                case LD:
                    regfile[execute_inst->reg_dst] = data_memory->Read(regfile[execute_inst->reg_srca]);
                    break;
                case ST:
                    data_memory->Write(regfile[execute_inst->reg_srca], regfile[execute_inst->reg_srcb]);
                    break;
                case ADD:
                    regfile[execute_inst->reg_dst] = regfile[execute_inst->reg_srca] + regfile[execute_inst->reg_srcb];
                    break;
                case IMM:
                    regfile[execute_inst->reg_dst] = execute_inst->imm;
                    break; 
                case JMP:
                    PC = execute_inst->target;
                    //no need to flush frontend (too few stages)                 
                    break;
                case BNE:
                    if (regfile[execute_inst->reg_srca] != regfile[execute_inst->reg_srcb]){
                        PC = execute_inst->target;
                        //flush frontend
                        frontend_flush = true;
                    }
                    break;
            }

	        server->MoveItem(execute2commit, execute_inst->GetDralItem());
	    }

	// Handle instruction queue and scheduling (scoreboard)
        //move instruction into scheduler 
        if (fetch_inst != NULL) { //guaranteed to not be full
            instruction_queue->Insert(fetch_inst);
        }

        //check if branch signal has been raised and need to flush
        if (frontend_flush == true){
            while (instruction_queue->IsEmpty() == false){
                schedule_inst = instruction_queue->Remove();
                server->DeleteItem(schedule_inst->GetDralItem());
                delete schedule_inst;
            }
            //no instruction is scheduled this cycle
            schedule_inst = NULL;
            server->SetNodeTag(schedule, "OCCUPANCY", instruction_queue->GetOccupancy());                        
        }
        else { //attempt to schedule instruction
            if (instruction_queue->IsEmpty() == false){
                schedule_inst = instruction_queue->Peek();

                //check if there is a RAW hazard, if so remove and delete
                if (scoreboard != -1 && (schedule_inst->reg_srca == scoreboard || schedule_inst->reg_srcb == scoreboard)){
                    schedule_inst = NULL;
                }
                else {
                    server->MoveItem(schedule2execute, schedule_inst->GetDralItem());
                    schedule_inst = instruction_queue->Remove();
                }
            }
            server->SetNodeTag(schedule, "OCCUPANCY", instruction_queue->GetOccupancy());                        
        }
        

	// Handle fetch of instruction
        if (frontend_flush != true && instruction_queue->IsFull() == false && PC < program_size) {
            fetch_inst = new INSTRUCTION_TOKEN_CLASS(PC, instruction_memory[PC].c_str(), server);

            PC = (PC+1) % program_size;
            server->MoveItem(fetch2schedule, fetch_inst->GetDralItem());

        }
        else {
            //no instruction fetched this cycle
            fetch_inst = NULL;
        }
    }

    //Disable the server
    server->TurnOff();

    //Clean up
    delete server;

    return 0;
}


