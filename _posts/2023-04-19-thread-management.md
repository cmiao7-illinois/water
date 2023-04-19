---
layout: post
toc: true
title: "C++ Concurrency(1) Thread Management"
categories: C++ Concurrency
tags: [C++, multithread]
author:
  - clmiao
---

1. Create a thread
2. RAII for exceptions
3. daemon threads
4. Reference of Arguments
5. Divide and Merge

## Create a thread

We can create a thread through:

1. functor: classes that are called like a function

2. Lambda function: more recommended than normal function
3. normal function: watch out pass the function address, instead an object/entity to a thread
4. a thread is created like std::thread(func, *args), not std::thread(func(), *args) 

Excemple:

```c++
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
```

## RAII for exceptions

RAII thread_guard allows program exit normally(clean contents one by one in stack).

If an exception occured in do_something_in_current_thread() part of function f(), the resources in thread_guard() will be destroyed still in destruct function ~thread_guard().

Excemple:

```c++
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
```

## daemon threads

Daemon threads, also called detached threads, run in the backend. Other thread will not interact with daemon threads, if we create a daemon thread in function f(), when the life of f() is over, the temporary variable will be destroyed. But the life of daemon threads will continue until the whole program(equals to main function) life is over.

Example:

```c++
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
```

If the program supports editing multiple files in one time, the prefered design is setting each file window as a single detached thread. Because the life is not determined by the function that created it, but will continue until the program life is over if not interrupted.

## Reference of Arguments

std::thread will first copy arguments and then pass them to callable functions. 

In this example, the update_function_for_widget function receive a reference of data, but std::thread will copy data first, and pass the reference of copied data instead the original data to this function. If we update data in update_function_for_widget, the changes will not be kept therefore. 

```c++
void update_data_for_widget(widget_id w, widget_data& data);
void oops_again(widget_id w){
    widget_data data;
    std::thread t(update_data_for_widget, w, data);
    display_status();
    t.join();
    process_widget_data(data);
}
```

We need std::ref to indicates pass original reference, not the reference of copied arguments to the function. 

```c++
std::thread t(update_data_for_widget, w, std::ref(data));
```

Or, we can pass pointer to realize the same effects. 

```c++
void update_data_for_widget(widget_id w, widget_data* data);

widget_data data;
std::thread t(update_data_for_widget, w, &data);
```

Even if the pointer is copied, the data poined to is always the same, so we can change the content of data in thread t.

Smart pointer is more safe than *, we can use unique_ptr, and pass the pointer to target function by std::move operation.

Example:

```c++
void update_data_for_widget(widget_id w, std::unique_ptr<widget_data> data);

widget_data data;
auto ptr = std::unique_ptr<widget_data>(&data);
std::thread t(update_data_for_widget, w, std::move(ptr));
```

unique_ptr is not copyable but moveable.

## Divide and Merge

When facing calculating-intensive task, we can divide the job to multi threads, finish them one by one, and merge these results at last. Give an example of calculating the sum of a continuous number:

```c++
template<typename Iterator, typename T>
struct accumulate_task{
    void operator()(Iterator first, Iterator last, T& result){
        result=std::accumulate(first, last, result);
    }
};
```

We define a functor to calculate the sum from Iterator first to last using std::accumlate function.

```c++
template<typename Iterator, typename T>
T parrallel_accumulate(Iterator first, Iterator last, T init){
    usigned long const length = std::distance(first, last);
    if(!length) return init;
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_cocurrency();
    unsigned long const num_threads=std::min(hardware_threads, max_threads);
 ---continue
}
```

The number of threads is better not too much, std::thread::hardware_cocurrency() checks the cpu of current system, and returns a maximum of threads. We can also estimate by designate min_per_thread(capacity of calculation for every thread) to 25, and give a resonable thread number.

```c++
template<typename Iterator, typename T>
T parrallel_accumulate(Iterator first, Iterator last, T init){
	---continue
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
```

The vector results record calculating results of threads by passing std::ref of every element of vector. Notice that the parallel_accumulate function itself could also be used to finish a job by '''accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads-1]
    );'''

This is because the result sentence will not be excuted untill all threads are joined, so the parallel_accumulate function has nothing to do unless we assign a calculating task to it.

After joining all threads, '''std::accumulate(results.begin(), results.end(), init);''' sentence collect results of all threads and finish the last step of calculation.



