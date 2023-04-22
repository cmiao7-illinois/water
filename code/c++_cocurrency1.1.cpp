#include <iostream>
#include <thread>
#include <string>

//functor for working in one thread
void do_something(int i){}
struct func{
    int& i;
    func(int& _i):i(_i){}
    void operator()(){
        for(unsigned int j=0;j<100;j++){
            do_something(i);
        }
    }
};