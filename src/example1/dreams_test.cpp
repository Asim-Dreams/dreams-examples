#define HOST_LINUX

#include <asim/syntax.h> 
#include <asim/dralServer.h>

#define MAX_CYCLES 4
int main(){
    
    //instantiate a dral server to generate files
    //string is name of output file
    DRAL_SERVER server = new DRAL_SERVER_CLASS("dral_trace");

    //Enable events to be written/captured by the server
    server->TurnOn(); 
 
    //Create a node and item
    UINT16 my_node = server->NewNode("my_node", -1);
    UINT32 my_item_0 = server->NewItem();

    //"clock" our system using a for loop. 'cycle' is the current cycle
    for (int curr_cycle= 0; curr_cycle < MAX_CYCLES; curr_cycle++){
        //Signal to the server that we are starting a new cycle for events to occur
        server->Cycle(curr_cycle);
        //Insert the item into the node, then remove it
        server->EnterNode(my_node, my_item_0, 0);
        server->ExitNode (my_node, my_item_0, 0);

    }

    //Disable the server
    server->TurnOff();

    //Clean up
    server->DeleteItem(my_item_0);
    delete server;

    return 0;
}


