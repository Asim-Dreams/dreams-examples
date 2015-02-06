#ifndef DATA_MEMORY_H
#define DATA_MEMORY_H

class DATA_MEMORY_CLASS
{

    public:

    private:

        int* data_array;
        int  data_array_size;

    public:

        DATA_MEMORY_CLASS(int new_size){
            data_array = new int[new_size];
            data_array_size = new_size;
        }

        int Read(int address){
            assert(address < data_array_size && address >= 0);

            return data_array[address];
        }

        void Write(int address, int data){
            assert(address < data_array_size && address >= 0);

            data_array[address] = data;
        }

    private:

};
typedef DATA_MEMORY_CLASS* DATA_MEMORY;

#endif
