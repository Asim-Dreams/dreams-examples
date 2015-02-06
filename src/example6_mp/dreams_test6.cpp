#define HOST_LINUX

#include <string.h>

#include <stdio.h> 
#include <asim/syntax.h> 
#include <asim/dralServer.h>
#include "data_memory.h"
#include "core.h"
#include <string.h>

#define MAX_CYCLES 25
#define NUM_CORES 2

int main(){

    // ******** Performance Model Declaration / Initialization
    int cycle = 0; //current cycle



    // ******** DRAL-Dreams Declaration / Initiatization ********
    //instantiate a dral server to generate files
    //string is name of output file
    auto server = new DRAL_SERVER_CLASS("dral_trace");

    //Enable events to be written/captured by the server
    server->TurnOn(); 
 
    //Instruction memory, each string is ASM instruction
    //this is the program we are wanting to run
    string instruction_memory[] = {
        "IMM R0, 128", // base addr 
        "IMM R1, 1", // stride
        "IMM R2, 2", // inner loop iterations
        "IMM R3, 1", // increment
        "IMM R4, 0", // counter                       
        "LD R5, R0", 
        "ADD R0, R0, R1", //inc address
        "ADD R4, R4, R3", //inc address
        "BNE R4, R2, I5",
        "JMP I4"};

    //Data Memory (global)
    auto data_memory = new DATA_MEMORY_CLASS(1024);// mem size (in 32-bit words)      

    auto cores = new CORE[NUM_CORES]; 
    for (int i=0; i<NUM_CORES; i++){
        cores[i] = new CORE_CLASS(i, instruction_memory, data_memory, server);
    }

    //Write a SetEnv in Dreams using a comment
    //- This pairs with the executable (perl) adf script below
    //- essentially passes parameters into the script
    //- NOTE: this must be before the adf is embedded 
    string env_var = "PERFMODEL_NUM_CPUS=" + std::to_string(NUM_CORES);
    server->Comment (0x05E7, env_var.c_str(), true); //5E7 -> 'SET'
    

    //Embed the ADF
    //- write the xml into a char string
    //- create comment with "magic number" 0xadf to identify as adf file
    FILE *f = fopen("test6_adf.pl", "rb");
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

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);

        //Clock cores
        for (int i=0; i<NUM_CORES; i++){
            cores[i]->Clock();
        }
      

     }

    //Disable the server
    server->TurnOff();

    //Clean up
    delete server;

    return 0;
}


