#include <iostream>
#include <thread>
#include <string>
//functor for working in one thread
struct func{
    int& i;
    func(int& _i):i(_i){}
    void operator()(){
        for(unsigned int j=0;j<100;j++){
            do_something(i);
        }
    }
};

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

void f(){
    int local_state=0;
    func my_func(local_state);
    std::thread t(my_func);
    thread_guard g(t);
    do_something_in_current_thread();
}

void edit_document(std::string const& filename){
    open_document_and_display(filename);
    while(!done_editing()){
        user_command cmd=get_user_input();
        if(cmd.type==open_new_document){
            std::string const new_file=get_filename();
            std::thread t(edit_document,new_file);
            t.detach();
        }
        else{
            process_user_input(cmd);
        }
    }
}

void update_data_for_widget(widget_id w, widget_data& data);
void oops_again(widget_id w){
    widget_data data;
    std::thread t(update_data_for_widget, w, data);
    display_status();
    t.join();
    process_widget_data(data);
}

template<typename Iterator, typename T>
struct accumulate_task{
    void operator()(Iterator first, Iterator last, T& result){
        result=std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T parrallel_accumulate(Iterator first, Iterator last, T init){
    usigned long const length = std::distance(first, last);
    if(!length) return init;
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_cocurrency();
    unsigned long const num_threads=std::min(hardware_threads, max_threads);
    
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads-1);
    
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);i++){
        Iterator block_end=block_start;
        std::advance(block_end, block_size);
        threads[i]=std::thread(
            accumulate_block<Itreator, T>(),
            block_start, block_end, std::ref(results[i])
        );
        block_start=block_end;
    }

    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads-1]
    );
    
    std::for_each(
        threads.begin(),
        threads.end(),
        std::mem_fn(&std::thread::join)
    );
    
    return std::accumulate(results.begin(), results.end(), init);
}