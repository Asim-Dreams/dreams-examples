#define HOST_LINUX

#include <string.h>

#include <asim/syntax.h> 
#include <asim/dralServer.h>

#define MAX_CYCLES 10
int main(){
    
    //instantiate a dral server to generate files
    //string is name of output file
    auto server = new DRAL_SERVER_CLASS("dral_trace");

    //Enable events to be written/captured by the server
    server->TurnOn(); 
 
    // Create the nodes of the pipeline

    auto fetch = server->NewNode("fetch", -1);
    auto execute = server->NewNode("execute", -1);
    auto commit = server->NewNode("commit", -1);
    auto done = server->NewNode("commit", -1);

    // Create the edges of the pipeline

    auto fetch2execute = server->NewEdge(fetch, execute, 1, 1, "fetch");
    auto execute2commit = server->NewEdge(execute, commit, 1, 1, "execute");
    auto commit2done = server->NewEdge(commit, done, 1, 1, "done");

    // Hack to initialize items to an illegal value

    auto fetch_inst = -1;
    auto execute_inst = -1;
    auto commit_inst = -1;
    auto done_inst = -1;

    //  'cycle' is the current cycle

    int cycle;
    int PC = 0;

    string memory[] = {"LD R1, A", 
                       "LD R2, B", 
                       "ADD R3,R1,R2", 
                       "LD R4, C", 
                       "SUB R5, R3, R4", 
                       "MUL R6, R1,R5", 
                       "ST R6, D",
                       "BR 0"};

    //"clock" our system using a for loop.

    for (cycle= 0; cycle < MAX_CYCLES; cycle++){

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);

	// Stupid systolic pipeline (so I can cheat on passing instructions around)

        // Handle instruction end of life

        done_inst = commit_inst;
        if (done_inst >= 0) {
	  server->DeleteItem(done_inst);
	}

	// Handle commit of instruction

        commit_inst = execute_inst;
        if (commit_inst >= 0) {
	  server->MoveItem(commit2done, commit_inst);
	}

	// Handle execute of instruction

        execute_inst = fetch_inst;
        if (execute_inst >= 0) {
	  server->MoveItem(execute2commit, execute_inst);
	}

	// Handle fetch of instruction

        fetch_inst = server->NewItem();
        server->SetItemTag (fetch_inst, "PC", PC);

	// We need to move dralServer into the 21st century...
        server->SetItemTag (fetch_inst, "Instruction", memory[PC].c_str());

        PC = (PC+1)%8;
        server->MoveItem(fetch2execute, fetch_inst);
    }

    //Disable the server
    server->TurnOff();

    //Clean up
    delete server;

    return 0;
}


