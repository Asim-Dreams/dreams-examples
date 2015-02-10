#ifndef DATA_CACHE_H
#define DATA_CACHE_H

#include <asim/dralServer.h>
#include "data_memory.h"
#include <math.h>

typedef struct
{
    bool valid;
    int tag;
    int data;
    int lru;
} DATA_CACHE_ENTRY;

//basic LRU cache
//implement write-through policy
//cache line is a single word (32-bit)
// word addressable only
class DATA_CACHE_CLASS
{

    public:
        static DRAL_SERVER dral_server;
        
    private:
        int ways;
        int sets;
        int max_lru;
        
        DATA_CACHE_ENTRY** array;
        DATA_MEMORY memory;
        UINT32 dral_node_hit;
        UINT32 dral_node_lru;


    public:
        DATA_CACHE_CLASS(int new_ways, int new_sets, DATA_MEMORY new_memory){
            sets = new_sets;
            ways = new_ways;
            max_lru = ways-1;
            
            memory = new_memory;

            // Define the nodes for the cache in the DRAL graph
            dral_node_hit = dral_server->NewNode("CacheHitMiss", 0);
            dral_node_lru = dral_server->NewNode("CacheLRU", 0);
            //Define the node's capacity (two dimensions)
            UINT32* cacheSize = new UINT32[2];
            cacheSize[0] = sets;
            cacheSize[1] = ways;
            dral_server->SetNodeLayout(dral_node_hit, 2, cacheSize);
            dral_server->SetNodeLayout(dral_node_lru, 2, cacheSize);


            array = new DATA_CACHE_ENTRY*[ways];
            for(int i=0; i<ways; i++){
                array[i] = new DATA_CACHE_ENTRY[sets];
            }
    
      
        }

        void Init(){
            for(int i=0; i<ways; i++){
                for(int j=0; j<sets; j++){                    
                    array[i][j].valid = false;
                    array[i][j].lru = max_lru;

                    //set the initial LRU in the DRAL graph
                    UINT32 position[2]; 
                    position[0] = j; 
                    position[1] = i;
                    dral_server->SetNodeTag(dral_node_lru, "LRU", max_lru, 2, position);

                }
            }

        }

        bool IsHit(int address) {
            bool is_hit = false;

            int way = GetWay(address);
            if (way != -1){
                is_hit = true;
            }
            return is_hit;
        }

        void Fill(int address) {
            int temp_set = AddressToSet(address);
            int temp_tag = AddressToTag(address);
            
            int found_way = -1;
            //try to find an invalid part of the cache
            for(int i=0; i< ways; i++){
                if (array[i][temp_set].valid == false){
                    found_way = i;             
                    break; //found empty space, leave loop
                }
            }

            //if none in set are empty, find the LRU way (LRU val == max

            if (found_way == -1){
                for(int i=0; i< ways; i++){
                    if (array[i][temp_set].lru == max_lru){
                        array[i][temp_set].valid = false;
                        found_way = i;
                        break; //found lru space, leave loop
                    }
                }
            
            }

            assert(found_way != -1);

            //place our new line in cache
            array[found_way][temp_set].valid = true;
            array[found_way][temp_set].tag = temp_tag;
            array[found_way][temp_set].data = memory->Read(address); //global memory?
                        
        }

        //create the address trace using DRAL items
        // - We create then immediately delete, as these items are not
        //      used anywhere else but to register the address stream
        void CreateAddressTrace(int address){
            int temp_set = AddressToSet(address);

            int address_item = dral_server->NewItem();
            dral_server->SetItemTag (address_item, "Address", address);
            dral_server->SetItemTag (address_item, "Set", temp_set);
            dral_server->DeleteItem (address_item);
        }
        
        void UpdateLRU(int set, int way){

            //update lru for those below old lru
            int old_lru = array[way][set].lru;
            for(int i=0; i< ways; i++){
                if (array[i][set].lru < old_lru){
                    array[i][set].lru++;
                }
            }

            // Use the DRAL node to update LRU in the set where we touched
            UINT32 position[2]; 
            for(int i=0; i< ways; i++){
                position[0] = set; 
                position[1] = i;
                dral_server->SetNodeTag(dral_node_lru, "LRU", array[i][set].lru, 2, position);
            }

            //update LRU for line we read
            array[way][set].lru = 0;
        }

        int Read(int address) { //also can be considered a "touch" which updates LRU
            CreateAddressTrace(address);

            bool is_hit = IsHit(address);

            if (is_hit == false){
                Fill(address);
            }
            
            int temp_way = GetWay(address);
            int temp_set = AddressToSet(address);
            
            UpdateLRU(temp_set, temp_way);

            UINT32 position[2]; 
            // Use the DRAL node tag "Hit" to specify where in the node we touched.
            //  by passing the set and way to node
            position[0] = temp_set; 
            position[1] = temp_way;
            dral_server->SetNodeTag(dral_node_hit, "HIT", is_hit, 2, position);

            //Create an item for the memory access, enter the node and then exit

              
            return array[temp_way][temp_set].data;
        }

        void Write(int address, int data) {
            CreateAddressTrace(address);

            bool is_hit = IsHit(address);

            if (is_hit == false){
                Fill(address);
            }


            int temp_way = GetWay(address);
            int temp_set = AddressToSet(address);

            UpdateLRU(temp_set, temp_way);

            //update line
            array[temp_way][temp_set].data = data;

            UINT32 position[2]; 
            // Use the DRAL node tag "Hit" to specify where in the node we touched.
            //  by passing the set and way to node
            position[0] = temp_set; 
            position[1] = temp_way;
            dral_server->SetNodeTag(dral_node_hit, "HIT", is_hit, 2, position);

        }

        int GetWay(int address){
            int found_way = -1;
            int temp_set = AddressToSet(address);
            int temp_tag = AddressToTag(address);
            for(int i=0; i< ways; i++){
                if (array[i][temp_set].valid == true && temp_tag == array[i][temp_set].tag){
                    found_way = i;
                    break; //found address, leave loop
                }
            }

            return found_way;
        }

        void Dump(){
            cout << "Data Cache" << endl;
            for(int j=0; j<sets; j++){
                cout << "  Set: " << j << endl;
                for(int i=0; i<ways; i++){
                    cout << "    Way: " << i << " -- (V: " << array[i][j].valid << "    LRU: " << array[i][j].lru << ")" << " -- Tag: " << array[i][j].tag << "    Data: " << array[i][j].data << endl;
                }
            }
            cout << endl;

        }

    private:

        int AddressToSet(int address) { return address & (sets-1); }
        int AddressToTag(int address) { return (address & (~(sets-1))) >> ((int) log2(sets)); }





};




#endif
