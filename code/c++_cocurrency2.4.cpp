#include <iostream>
#include <mutex>
#include <thread>

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