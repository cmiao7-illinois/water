---
layout: post
toc: true
title: "C++ Concurrency(2) Thread Shared Data"
categories: C++ Concurrency
tags: [C++, multithread]
author:
  - clmiao
---

1. Thread-safe stack;
2. Lock granularity;
3. Dead lock;
4. Singleton and std::call_once;
5. Recursive lock; 
6. Hierarchical mutex.

## Thread-safe stack

STL does not support thread-safety defaultly, we have to design a thread-safe stack if we need it. The core is the class function pop() and top(): top() will copy the top-data and return to receiver, and pop() will clear the top-data. But when multi-threads, if we still divide these two functions, the safety will not be promised.

Assuming we have threads t1 and t2, t1 has get the top() but hasn't pop() but t2 got the top() also, the same top() value will be processed twice without any error, which will not be easily noticed. 

Therefore, we combine top() and pop() by passing a reference to receive the top-data, or return the pointer of top-data directly, which will get data and pop data with locking one time. 

Example:

```c++
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex mtx;
public:
    explicit threadsafe_stack() : data(std::stack<T>()) {}
    threadsafe_stack(std::stack<T> &&_data)
    {
        std::unique_lock<std::mutex> lck(mtx);
        this->data = std::move(_data);
    }
    void push(T value)
    {
        std::unique_lock<std::mutex> lck(mtx);
        data.push(value);
    }
    std::shared_ptr<T> pop()
    {
        std::unique_lock<std::mutex> lck(mtx);
        if (data.empty())
            return nullptr;
        auto res = std::make_shared<T>(data.top());
        data.pop();
        return res;
    }
    bool pop(T &value)
    {
        std::unique_lock<std::mutex> lck(mtx);
        if (data.empty())
            return 0;
        value = data.top();
        data.pop();
        return 1;
    }
};
```

It reminds us that to design thread-safe data-structure, we need to revise the interfaces and merge some functions to decrease potential conflicts.

Function call:

```c++
template <typename T>
void move_in1(threadsafe_stack<T> &t_s1, threadsafe_stack<T> &t_s2) {
    int value = 0;
    while (t_s2.pop(value))
    {
        std::cout << "t1: " << value << std::endl;
        t_s1.push(value);
    }
}

template <typename T>
void move_in2(threadsafe_stack<T> &t_s1, threadsafe_stack<T> &t_s2) {
    std::shared_ptr<T> ptr = t_s2.pop();
    while (ptr != nullptr)
    {
        std::cout << "t2: " << *ptr << std::endl;
        t_s1.push(*ptr);
        ptr = t_s2.pop();
    }
}
```

## Lock granularity (锁粒度)

Lock granularity refers to the length of locked code, or the number of operations. Our developers are ought to decrease the lock granularity to add time of threads' co-working. 

One solution to decrease the lock granularity is to lock mutex only when there are conflicts, like get_data and write_data. The process of calculating and processing data should not be locked.

Example:

```c++
struct threadsafe_Data {
private:
    std::mutex mtx;
    std::vector<int> data;
  public:
    explicit threadsafe_Data();
    std::vector<int> get_data();
    void write_data(std::vector<int> &data_chunk);
};

void process_data_chunk(std::vector<int>& data_chunk);

void get_and_process_data() {
    threadsafe_Data data;
    auto data_chunk = data.get_data();
    process_data_chunk(data_chunk);
    data.write_data(data_chunk);
}
```

You may notice that by setting std::mutex as class member, we can realize decreasing the lock granularity easily. We only need not to lock mutex in process_data function, in which method the lock granularity will be decreased easily.

## Dead lock

Dead lock is a common dilemma in multi-threads. We need to clear that dead lock is not the same mutex is locked for two times by two threads, that action is allowed for std::lock. 

Dead lock is two mutex is locked by two threads, while both threads are waiting for another to release the mutex to continue own job. So the conditions for dead lock is that one operation(or function) is related to two mutexs. Like the operation of swapping, the easiest and most common method to avoid dead lock is confirming the locking order of multiple mutexs.

Example:

```c++
#include <iostream>
#include <mutex>
#include <thread>

struct bank_account {
    explicit bank_account(int balance):m_balance(balance){}
    int m_balance;
    std::mutex m_mtx;
};

void transfer(bank_account& a, bank_account& b, int amount){
    std::unique_lock<std::mutex> lck1(a.m_mtx, std::defer_lock);
    std::unique_lock<std::mutex> lck2(b.m_mtx, std::defer_lock);
    std::lock(lck1, lck2);
    //equals to the following code:
    // std::lock(a.m_mtx, b.m_mtx);
    // std::lock_guard<std::mutex> lck1(a.m_mtx, std::adopt_lock);
    // std::lock_guard<std::mutex> lck2(b.m_mtx, std::adopt_lock);
    a.m_balance -= amount;
    b.m_balance += amount;
}

int main(){
    bank_account a_acount(100);
    bank_account b_account(10);
    std::thread t1(transfer, std::ref(a_acount), std::ref(b_account), 20);
    std::thread t2(transfer, std::ref(a_acount), std::ref(b_account), -10);
    t1.join();
    t2.join();
    std::cout << a_acount.m_balance << " " << b_account.m_balance << std::endl;
    return 0;
}
```

Many people, including me, thought std::lock_guard or std::unique_lock is the action of lock. That is wrong. Actually, lock_guard and unique_lock are mutex management classes like smart pointer, which means the locking action is not necessarily executed in initialization. The std::lock function is the real locking operation function.

std::unique_lock lck2(b.m_mtx, ...);
... includes 4 categories:
1. none: initialze and lock at the same time;
2. std::defer_lock: initialize and not lock, the locking operation will in the next few lines;
3. std::adopt_lock: initialize and not lock, the locking operation has been excuted in the last few lines;
4. std::try_to_lock: initialize and try to lock, and continue to next lines' coding no matter succeed or not.

Therefor, the code part in the demo is the same to each other, implemented by std::unique_lock and std::lock_guard seperately.


## Hierarchical mutex

Hierarchical_mutex sets a rule for locking order to avoid dead lock by pre-set the argument of importance of mutex. 

Example:

```c++
hierarchical_mutex high_level_mutex(10000); // 1
hierarchical_mutex low_level_mutex(5000);  // 2

int do_low_level_stuff();
int low_level_func()
{
  std::lock_guard<hierarchical_mutex> lk(low_level_mutex); // 3
  return do_low_level_stuff();
}

void high_level_stuff(int some_param);
void high_level_func()
{
  std::lock_guard<hierarchical_mutex> lk(high_level_mutex); // 4
  high_level_stuff(low_level_func()); // 5
}
std::thread t(high_level_func);
```

By setting the importance of 10000 to high-level stuff, 5000 to low level stuff, locking operations are forced to be excuted at first high_level_mutex, then low_level_mutex, which will avoid dead lock effectively.

Attention that hierarchical_mutex is not offer by standard libiraries, the implementation of it is personalized. I will not disscuss it because its high complexity, all we need to do is knowing how to use it.

## Singleton and std::call_once

Singleton: If we open a file, and open it again, we'll find the window will not be created again. Singleton means we have no need to create more than one instance. Attention that if we initialize the class first, then passing it to different threads, the same effects will also be realized. But many times we have to initialize instances in different threads, that's when singleton stands out.

Example:

```c++
class Singleon {
  private:
    Singleon() {}
    static Singleon *instance;

  public:
    static Singleon *getInstance() {
        if (instance == nullptr) {
            instance = new Singleon();
            static Deleter del;
        }
        return instance;
    }
    class Deleter {
      public:
        ~Deleter() {
            if (Singleon::instance) {
                delete Singleon::instance;
                Singleon::instance = nullptr;
            }
        }
    };
    void func() { std::cout << "test passed" << std::endl; }
};
```
The instance pointer is static, different functions and threads operates on this pointer together. ~Deleter is defined to destruct this Singleton, because the class will be destructed automatically after one thread is over. If we not initialize a static deleter in createInstance() function, the instance will be destructed many times.



As to a threadsafe-Singleton, the std::call_once will execute the passed function(createInstance) once even in many threads. The std::once_falg will record whether the call_once has been executed and change its status. And also, the std::mutex need to be introduced to solve conflicts in class functions.

Example and calling:

```c++
class threadsafe_Singleton {
  private:
    static threadsafe_Singleton *instance;
    inline static std::once_flag flag;
    inline static std::mutex mtx;
    threadsafe_Singleton() {}
    static void createInstance() {
        instance = new threadsafe_Singleton();
        static Deleter del;
        std::cout << "create an instance" << std::endl;
    }

  public:
    static threadsafe_Singleton *getInstance() {
        std::call_once(threadsafe_Singleton::flag, createInstance);
        return instance;
    }
    class Deleter {
      public:
        ~Deleter() {
            if (threadsafe_Singleton::instance) {
                delete threadsafe_Singleton::instance;
                threadsafe_Singleton::instance = nullptr;
            }
        }
    };
    void func() {
        std::lock_guard<std::mutex> lck(threadsafe_Singleton::mtx);
        std::cout << "test passed" << std::endl;
    }
};

threadsafe_Singleton *threadsafe_Singleton::instance = nullptr;

int main() {
    std::thread t1([] {
        threadsafe_Singleton *ptr1 = threadsafe_Singleton::getInstance();
        ptr1->getInstance();
        ptr1->func();
    });
    std::thread t2([] {
        threadsafe_Singleton *ptr2 = threadsafe_Singleton::getInstance();
        ptr2->getInstance();
        ptr2->func();
    });

    t1.join();
    t2.join();
    return 0;
}
```



## Recursive mutex

Std::recursive_mutex allows to be locked twice by one thread. There is one posiblity that we use std::recursive like:

```c++
struct bank_account {
    std::recursive m_mtx;
    int m_balance;
};
```

Is that wrong because different threads will lock the same mtx more than one time, because it's recursive? Actually not, std::recursive allows to be locked more than one time only within one threads, not cross different threads. Which means thread A could lock mtx more than one time, but thread B need to wait until both two locks of thread a to be released. 

std::recursive_mutex is useful when one mutex need to be locked more than once.

