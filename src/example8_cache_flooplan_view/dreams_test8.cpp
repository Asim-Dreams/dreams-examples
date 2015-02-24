#define HOST_LINUX

#include <string.h>

#include <stdio.h> 
#include <asim/syntax.h> 
#include <asim/dralServer.h>
#include "instruction.h"
#include "data_memory.h"
#include "cache.h"

#define MAX_CYCLES 400

DRAL_SERVER DATA_CACHE_CLASS::dral_server = NULL;

int main(){

    // ******** Performance Model Declaration / Initialization
    int cycle = 0; //current cycle



    // ******** DRAL-Dreams Declaration / Initiatization ********
    //instantiate a dral server to generate files
    //string is name of output file
    auto server = new DRAL_SERVER_CLASS("dral_trace");

    //pass server to cache
    DATA_CACHE_CLASS::dral_server = server;

    //Enable events to be written/captured by the server
    server->TurnOn(); 
 

    // ******** CPU State Declaration / Initialization ********
    int PC = 0;             //instruction pointer for the CPU

    // Pipeline stage latches
    INSTRUCTION_TOKEN fetch_inst = NULL;
    INSTRUCTION_TOKEN execute_inst = NULL;
    INSTRUCTION_TOKEN commit_inst = NULL;
    INSTRUCTION_TOKEN done_inst = NULL;

    //Instruction memory, each string is ASM instruction
    //this is the program we are wanting to run


    //memory streaming program                   
    string instruction_memory[] = {
                       "IMM R0, 0", // base addr 
                       "IMM R1, 3", // stride
                       "IMM R2, 31", // Mask
                       "LD R3, R0", 
                       "ADD R0,R0,R1", 
                       "AND R0,R0,R2",
                       "JMP 3"};
    

    //Data Memory
    auto data_memory = new DATA_MEMORY_CLASS(1024);// mem size (in 32-bit words) 

    //Cache Memory
    auto data_cache = new DATA_CACHE_CLASS(4, 8, data_memory); // ways, sets, 

    //Register File
    auto regfile = new int[8]; // rf size is 8 entries



    //Embed the ADF
    //- write the xml into a char string
    //- create comment with "magic number" 0xadf to identify as adf file
    FILE *f = fopen("test8_adf.xml", "rb");
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
    //initialize cache after starting activity collection
    //    so that initialized LRU data can be written into trace
    data_cache->Init();
    for (cycle= 0; cycle < MAX_CYCLES; cycle++){

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);
	// Stupid systolic pipeline (so I can cheat on passing instructions around)

        // Handle instruction end of life

        done_inst = commit_inst;
        if (done_inst != NULL) {
            delete done_inst;
	    }

	// Handle commit of instruction

        commit_inst = execute_inst;

	// Handle execute of instruction

        execute_inst = fetch_inst;
        if (execute_inst != NULL) { 
            int address;

            switch (execute_inst->op){
                case LD:
                    address = regfile[execute_inst->reg_srca];
                    regfile[execute_inst->reg_dst] = data_cache->Read(address);
                    break;
                case ST:
                    address = regfile[execute_inst->reg_srca];
                    data_cache->Write(regfile[execute_inst->reg_srca], regfile[execute_inst->reg_srcb]);
                    break;
                case ADD:
                    regfile[execute_inst->reg_dst] = regfile[execute_inst->reg_srca] + regfile[execute_inst->reg_srcb];
                    break;
                case AND:
                    regfile[execute_inst->reg_dst] = regfile[execute_inst->reg_srca] & regfile[execute_inst->reg_srcb];
                    break;
                case IMM:
                    regfile[execute_inst->reg_dst] = execute_inst->imm;
                    break; 
                case JMP:
                    PC = execute_inst->target;
                    break;
                case BNE:
                    if (regfile[execute_inst->reg_srca] != regfile[execute_inst->reg_srcb]){
                        PC = execute_inst->target;
                    }
                    break;
            }

	   }

	// Handle fetch of instruction
        fetch_inst = new INSTRUCTION_TOKEN_CLASS(PC, instruction_memory[PC].c_str());

        PC = (PC+1);
       
    }

    //Disable the server
    server->TurnOff();

    //Clean up
    delete server;

    return 0;
}


