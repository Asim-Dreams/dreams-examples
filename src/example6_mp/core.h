#ifndef CORE_H
#define CORE_H

#include "instruction.h"
#include <string.h>

class CORE_CLASS {
    
    public:

    private:

        int id;

        // ******** CPU State Declaration / Initialization ********


        int PC;             //instruction pointer for the CPU

        // Pipeline stage latches
        INSTRUCTION_TOKEN fetch_inst;
        INSTRUCTION_TOKEN execute_inst;
        INSTRUCTION_TOKEN commit_inst;
        INSTRUCTION_TOKEN done_inst;

        //Register File
        int* regfile;

        string* instruction_memory;

        DATA_MEMORY data_memory;

        //DRAL data structures
        DRAL_SERVER server;

         // Create the nodes of the pipeline
        UINT16 fetch;
        UINT16 execute;
        UINT16 commit;
        UINT16 done;

        // Create the edges of the pipeline
        UINT16 fetch2execute;
        UINT16 execute2commit;
        UINT16 commit2done;
       

    public:
        CORE_CLASS(int new_id, string* new_instruction_memory, DATA_MEMORY new_data_memory, DRAL_SERVER new_server) {
            id = new_id;
            PC = 0;
            regfile  = new int[8]; // rf size is 8 entries

            data_memory = new_data_memory;
            instruction_memory = new_instruction_memory;
            server = new_server;

            fetch_inst = NULL;
            execute_inst = NULL;
            commit_inst = NULL;
            done_inst = NULL;

            // Create the nodes of the pipeline
            string name_string = "FETCH" + std::to_string(id);
            fetch = server->NewNode(name_string.c_str(), 0);

            name_string = "EXECUTE" + std::to_string(id);
            execute = server->NewNode(name_string.c_str(), 0);
            
            name_string = "COMMIT" + std::to_string(id);
            commit = server->NewNode(name_string.c_str(), 0);
            
            name_string = "DONE" + std::to_string(id);
            done = server->NewNode(name_string.c_str(), 0);

            // Create the edges of the pipeline
            name_string = "FETCH" + std::to_string(id);
            fetch2execute = server->NewEdge(fetch, execute, 1, 1, name_string.c_str());

            name_string = "EXECUTE" + std::to_string(id);
            execute2commit = server->NewEdge(execute, commit, 1, 1, name_string.c_str());

            name_string = "COMMIT" + std::to_string(id);
            commit2done = server->NewEdge(commit, done, 1, 1, name_string.c_str());
          
        }

        void Clock(){

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
                        address = regfile[execute_inst->reg_srca];
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
                            //no need to flush frontend (too few stages)
                        }
                        break;
                }

                server->MoveItem(execute2commit, execute_inst->GetDralItem());
            }

            // Handle fetch of instruction
            fetch_inst = new INSTRUCTION_TOKEN_CLASS(PC, instruction_memory[PC].c_str(), server);

            PC = (PC+1);
            server->MoveItem(fetch2execute, fetch_inst->GetDralItem());
            
        }

    private:
};
typedef CORE_CLASS* CORE;

#endif

