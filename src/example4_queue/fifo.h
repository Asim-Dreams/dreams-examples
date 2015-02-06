#ifndef FIFO_H
#define FIFO_H


//basic FIFO Queue
template <class T> 
class FIFO_CLASS
{

    public:
        
    private:
        int capacity;
        int size;
        int head;
        int tail;

        T* buffer;


    public:
        FIFO_CLASS(int new_capacity){
            buffer = new T[new_capacity];
            head = 0;
            tail = 0;
            size = 0;
            capacity = new_capacity;
        }

        int GetOccupancy(){
            return size;
        }

        bool IsEmpty(){
            return (size == 0);
        }

        bool IsFull(){
            return (size == capacity);
        }

        void Insert(T value){
            buffer[tail] = value;
            tail = (tail + 1) % capacity;
            size++;
        }

        T Remove(){
            T ret_val = buffer[head];
            buffer[head] = NULL;
            head = (head + 1) % capacity;
            size--;
            return ret_val;
        }

        T Peek(){
            T ret_val;
            if (IsEmpty() == true){
                ret_val = NULL;
            }
            else{
                ret_val = buffer[head];
            }
            return ret_val;
        }

    private:


};


#endif
