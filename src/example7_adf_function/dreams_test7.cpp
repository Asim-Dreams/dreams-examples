#define HOST_LINUX

#include <string.h>

#include <stdio.h> 
#include <asim/syntax.h> 
#include <asim/dralServer.h>
#include "instruction.h"
#include "data_memory.h"

#define MAX_CYCLES 25

void add_adf(DRAL_SERVER_CLASS *server);

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
    auto execute = server->NewNode("EXECUTE", 0);
    auto commit = server->NewNode("COMMIT", 1);
    auto done = server->NewNode("DONE", 2);

    // Create the edges of the pipeline
    auto fetch2execute = server->NewEdge(fetch, execute, 1, 1, "FETCH");
    auto execute2commit = server->NewEdge(execute, commit, 1, 1, "EXECUTE");
    auto commit2done = server->NewEdge(commit, done, 1, 1, "COMMIT");



    // ******** CPU State Declaration / Initialization ********
    int PC = 0;             //instruction pointer for the CPU
    bool branch_taken; 

    // Pipeline stage latches
    INSTRUCTION_TOKEN fetch_inst = NULL;
    INSTRUCTION_TOKEN execute_inst = NULL;
    INSTRUCTION_TOKEN commit_inst = NULL;
    INSTRUCTION_TOKEN done_inst = NULL;

    //Instruction memory, each string is ASM instruction
    //this is the program we are wanting to run


    //memory streaming program                   
    string instruction_memory[] = {
                       "IMM R0, 128", // base addr 
                       "IMM R1, 1", // stride
                       "LD R2, R0", 
                       "ADD R0,R0,R1", 
                       "JMP 2"};
    
    //Data Memory
    auto data_memory = new DATA_MEMORY_CLASS(1024);// mem size (in 32-bit words) 
     
    //Register File
    auto regfile = new int[8]; // rf size is 8 entries


    //Embed the ADF
    add_adf(server);

    // ******** Performance Model Execution ********
    //"clock" our system using a for loop.
    server->StartActivity(0);   
   
    for (cycle= 0; cycle < MAX_CYCLES; cycle++){

        //Reset the "branch taken" flag
        branch_taken = false;

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);


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

        execute_inst = fetch_inst;
        if (execute_inst != NULL) { 
            int address;

            switch (execute_inst->op){
                case LD:
                    regfile[execute_inst->reg_dst] = data_memory->Read(execute_inst->reg_srca);
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
                    branch_taken = true;
                    break;
                case BNE:
                    if (regfile[execute_inst->reg_srca] != regfile[execute_inst->reg_srcb]){
                        PC = execute_inst->target;
                        branch_taken = true;
                    }
                    break;
            }

	        server->MoveItem(execute2commit, execute_inst->GetDralItem());
	    }

	// Handle fetch of instruction
        if (branch_taken == false){
            fetch_inst = new INSTRUCTION_TOKEN_CLASS(PC, instruction_memory[PC].c_str(), server);

            PC = (PC+1);
            server->MoveItem(fetch2execute, fetch_inst->GetDralItem());
        }
        else {
            fetch_inst = NULL;
        }
       
    }

    //Disable the server
    server->TurnOff();

    //Clean up
    delete server;

    return 0;
}


