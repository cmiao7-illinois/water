#include <thread>
#include <mutex>

struct threadsafe_Data {
private:
    std::vector<int> data;
  public:
    explicit threadsafe_Data();
    std::vector<int> get_data();
    void write_data(std::vector<int> &data_chunk);
};

std::mutex mtx;
void process_data_chunk(std::vector<int>& data_chunk);
void get_and_process_data() {
    threadsafe_Data data;
    std::unique_lock<std::mutex> lck(mtx);
    auto data_chunk = data.get_data();
    lck.unlock();
    process_data_chunk(data_chunk);
    lck.lock();
    data.write_data(data_chunk);
}