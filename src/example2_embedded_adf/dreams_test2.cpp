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


    //Embed the ADF
    //- write the xml into a char string
    //- create comment with "magic number" 0xadf to identify as adf file
    FILE *f = fopen("test2_adf.xml", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* string = new char[fsize + 1];
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0; //null terminate
    server->Comment (0x0adf, string, true);

 
    // Create the nodes of the pipeline

    auto fetch = server->NewNode("FETCH", 0);
    auto execute = server->NewNode("EXECUTE", 1);
    auto commit = server->NewNode("COMMIT", 2);
    auto done = server->NewNode("DONE", 3);

    // Create the edges of the pipeline

    auto fetch2execute = server->NewEdge(fetch, execute, 1, 1, "FETCH");
    auto execute2commit = server->NewEdge(execute, commit, 1, 1, "EXECUTE");
    auto commit2done = server->NewEdge(commit, done, 1, 1, "COMMIT");

    // Hack to initialize items to an illegal value

    auto fetch_inst = -1;
    auto execute_inst = -1;
    auto commit_inst = -1;
    auto done_inst = -1;

    //  'cycle' is the current cycle

    int cycle;

    server->StartActivity(0);
    
    //"clock" our system using a for loop.

    for (cycle= 0; cycle < MAX_CYCLES; cycle++){

        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(cycle);


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

	// Handle fetch/creation of tokens

        fetch_inst = server->NewItem();
        server->SetItemTag (fetch_inst, "START_CYCLE", cycle);
        server->MoveItem(fetch2execute, fetch_inst);
    }

    //Disable the server and stop writing trace file
    //Comment (UINT32 magicNum, const char comment [], bool persistent=false);
    server->TurnOff();
    
    //Clean up
    delete server;

    return 0;
}


