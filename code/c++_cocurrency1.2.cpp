#include <iostream>
#include <thread>
#include <string>

//RAII allows auto destruct
class thread_guard{
private:
    std::thread& t;
public:
    explicit thread_guard(std::thread& _t):t(_t){}
    ~thread_guard(){
        if(t.joinable()){t.join();}
    }
    thread_guard(thread_guard const&)=delete;
    thread_guard& operator=(thread_guard const&)=delete;
};

struct func {
    explicit func(int state){}
};

void do_something_in_current_thread(){}

void f() {
    int local_state=0;
    func my_func(local_state);
    std::thread t(my_func);
    thread_guard g(t);
    do_something_in_current_thread();
}
