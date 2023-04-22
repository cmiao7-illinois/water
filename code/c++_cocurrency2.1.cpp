#include <iostream>
#include <thread>
#include <mutex>
#include <stack>

template <typename T>
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

    void print_elements()
    {
        std::unique_lock<std::mutex> lck(mtx);
        while (!data.empty())
        {
            std::cout << data.top() << std::endl;
            data.pop();
        }
    }
};

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

int main()
{
    std::stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    s.push(4);
    threadsafe_stack<int> t_s1{};
    threadsafe_stack<int> t_s2(std::move(s));
    std::thread t1(move_in1<int>, std::ref(t_s1), std::ref(t_s2));
    std::thread t2(move_in2<int>, std::ref(t_s1), std::ref(t_s2));
    t1.join();
    t2.join();
    t_s1.print_elements();
    return 0;
}