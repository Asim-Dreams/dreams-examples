#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef enum {
    //Memory instructions
    LD,
    ST,
    //ALU instructions
    ADD,
    IMM, //Load immediate into dest register
    //Control instructions
    JMP, // unconditional jump -> absolute pc
    BNE, //conditional jump (branch not equal) -> absolute pc
    NUM
} OPCODE;

const char* opcodes[] = { 
    "LD", 
    "ST",
    "ADD", 
    "IMM", 
    "JMP",
    "BNE"
};

class INSTRUCTION_TOKEN_CLASS
{

    public:
        OPCODE op;
        int    reg_dst;
        int    reg_srca;
        int    reg_srcb; 
        int    imm;  
        int    target;  
        int    pc;

    private:

    private:
        DRAL_SERVER dral_server;
        UINT32 dral_item;


    public:
        INSTRUCTION_TOKEN_CLASS(int new_pc, const char* new_instr, DRAL_SERVER new_server)
        : reg_dst(-1), reg_srca(-1), reg_srcb(-1), imm(-1), target(-1) {
            
            //set local performance model information
            pc = new_pc;
            //parse instructions to get other variables
            // we need to first get an opcode, then either an immediate or operands

            //slurp in opcode
            int   curr = 0, i = 0, length = 0;
            char temp[8];
            while (new_instr[curr]>='A' && new_instr[curr]<='Z'){
                temp[i] = new_instr[curr];
                curr++;
                i++;
            }
            temp[i] = '\0'; //finish string with null character
            length = i+1; //record length of string


            op = ParseOpcode(temp, length);

            switch (op){
                case LD:
                    reg_dst = ParseNextVal(new_instr, curr); 
                    reg_srca = ParseNextVal(new_instr, curr); 
                    break;
                case ST:
                    reg_srca = ParseNextVal(new_instr, curr); 
                    reg_srcb = ParseNextVal(new_instr, curr); 
                    break;
                case ADD:
                    reg_dst = ParseNextVal(new_instr, curr); 
                    reg_srca = ParseNextVal(new_instr, curr);                     
                    reg_srcb = ParseNextVal(new_instr, curr);                     
                    break;
                case IMM:
                    reg_dst = ParseNextVal(new_instr, curr); 
                    imm = ParseNextVal(new_instr, curr);
                    break; 
                case JMP:
                    target = ParseNextVal(new_instr, curr); 
                    break;
                case BNE:
                    reg_srca = ParseNextVal(new_instr, curr);                     
                    reg_srcb = ParseNextVal(new_instr, curr);
                    target = ParseNextVal(new_instr, curr);                     
                    break;
                default:
                    assert(0);
                    break;
           }

    
            //set up dreams server information
            dral_server = new_server;
            dral_item = dral_server->NewItem();
            dral_server->SetItemTag (dral_item, "PC", new_pc);
            dral_server->SetItemTag (dral_item, "Instruction", new_instr);
            dral_server->SetItemTag (dral_item, "OPCODE_STRING", temp);
            dral_server->SetItemTag (dral_item, "OPCODE_NUM", op);

        }

        void SetDependency(INSTRUCTION_TOKEN_CLASS* instr){
            //Create a set of values (with set size = 1)
            INT32* val_array = new INT32[1];
            val_array[0] = instr->GetDralItem();
            dral_server->SetItemTag (dral_item, "ANCESTOR", 1, val_array, false);
        }

        UINT32 GetDralItem(){
	        return dral_item;
        }

        void Dump(){
            cout << "Instruction: " << dral_item << endl;
            cout << "  PC: " << pc << endl;
            cout << "  Opcode: " << opcodes[op] << endl;
            switch (op){
                case LD:
                    cout << "  DestReg: R" << reg_dst << endl;
                    cout << "  SrcReg: R" << reg_srca << endl;
                    break;
                case ST:
                    cout << "  SrcRegA: R" << reg_srca << endl;
                    cout << "  SrcRegB: R" << reg_srcb << endl;
                    break;
                case ADD:
                    cout << "  DestReg: R" << reg_dst << endl;
                    cout << "  SrcRegA: R" << reg_srca << endl; 
                    cout << "  SrcRegB: R" << reg_srcb << endl; 
                    break;
                case IMM:
                    cout << "  DestReg: R" << reg_dst << endl;
                    cout << "  Immediate: " << imm << endl;
                    break; 
                case JMP:
                    cout << "  Target: I" << target << endl;
                    break;
                 case BNE:
                    cout << "  Target: I" << target << endl;
                    cout << "  SrcRegA: R" << reg_srca << endl; 
                    cout << "  SrcRegB: R" << reg_srcb << endl; 
                    break;
           }

            cout << endl;
        }

    private:
        OPCODE ParseOpcode(char* temp, int length){
            //search for opcode match with parsed substring
            bool match = false;
            int  match_index = -1;
            int  index = 0;
            while (match != true && index < OPCODE::NUM){  
                for (int i=0; i<length; i++){
                    //as long as the substrings match continue
                    //otherwise, opcode does not match, move to next opcode string
                    if (temp[i] == opcodes[index][i]){
                        match = true;
                    }
                    else {
                        match = false;
                        break;
                    }
                }

                //check if we found the opcode we wanted
                if (match == true){
                    match_index = index;
                    break;
                }
                index++;
            }
            
            return (OPCODE) match_index;

        }

        //update "curr" point as well -> note passed by reference
        int ParseNextVal(const char* instr, int& curr){
            char temp_reg[16];

            //skip through the char of the string to find the next number
            while ((instr[curr] < '0' || instr[curr] > '9') && instr[curr] != '-'){
                curr++;
            }
            //slurp in the number
            int index=0;
            while (instr[curr] == '-' || (instr[curr] >= '0' && instr[curr] <= '9')){
                temp_reg[index] = instr[curr];
                index++;
                curr++;
            }
            temp_reg[index] = '\0'; // null terminate

            return atoi(temp_reg);            
        }


};

typedef INSTRUCTION_TOKEN_CLASS* INSTRUCTION_TOKEN;
#endif 

