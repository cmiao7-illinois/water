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