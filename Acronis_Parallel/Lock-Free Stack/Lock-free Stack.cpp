#include <iostream>
#include <errno.h>
#include <stdexcept>
#include <memory>
#include <atomic>
#include <thread>
#include <ctime>
#include <vector>
#include <functional>

struct hazard_pointer
{
    std::atomic<std::thread::id> id_;
    std::atomic<void*>           ptr_;
};

size_t max_num_hazard_ptrs = 100;
hazard_pointer* arr_hazard_pointers;

void init_hazard_pointers_environment(size_t num_threads)
{
    arr_hazard_pointers = new hazard_pointer[num_threads + 1];
    max_num_hazard_ptrs = num_threads + 1;
}

void destr_hazard_pointers_environment()
{
    delete[] arr_hazard_pointers;
}

class hazard_owner
{
private:

    hazard_pointer* hazard_ptr_;

public:

    hazard_owner(hazard_owner const&) = delete;
    hazard_owner operator=(hazard_owner const&) = delete;
    hazard_owner() :
        hazard_ptr_(nullptr)
    {
        if (arr_hazard_pointers == nullptr)
            throw std::runtime_error("Empty hazard ptr array!. Firstly, you need to init hazard pointers\n");

        auto curr_id = std::this_thread::get_id();
        for (size_t i = 0; i < max_num_hazard_ptrs; i++)
        {
            std::thread::id arr_value;
            if (arr_hazard_pointers[i].id_.compare_exchange_strong(arr_value, curr_id))
            {
                hazard_ptr_ = &arr_hazard_pointers[i];
                break;
            }
        }
        if (hazard_ptr_ == nullptr) // all pointers finished
            throw std::runtime_error("Array of hazard pointer is smaller that array of threads\n");

    }

    ~hazard_owner()
    {
        hazard_ptr_->ptr_.store(nullptr);
        hazard_ptr_->id_.store(std::thread::id());
    }

    std::atomic<void*>& get_ptr()
    {
        return hazard_ptr_->ptr_;
    }
};

std::atomic<void*>& get_hazard_pointer_for_current_thread()
{
    thread_local static hazard_owner hazard_manager;
    return hazard_manager.get_ptr();
}

bool is_hazard(void* ptr)
{
    if (arr_hazard_pointers == nullptr)
        throw std::runtime_error("Empty hazard ptr array!. Firstly, you need to init hazard pointers\n");

    if (ptr == nullptr)
        return false;

    for (size_t i = 0; i < max_num_hazard_ptrs; i++)
    {
        if (arr_hazard_pointers[i].ptr_.load() == ptr)
            return true;
    }

    return false;
}

template<typename T>
void do_delete(void* ptr)
{
    delete static_cast<T*>(ptr);
}

struct data_to_delete
{
    void* data_;
    std::function<void(void*)> deleter_;
    data_to_delete* next_;

    template<typename T>
    data_to_delete(T* ptr) :
        data_(ptr),
        deleter_(&do_delete<T>),
        next_(nullptr)
    {}

    ~data_to_delete()
    {
        deleter_(data_);
    }
};

std::atomic<data_to_delete*> delete_list;

void add_to_delete_list(data_to_delete* node)
{
    node->next_ = delete_list.load(std::memory_order_acquire);
    while (!delete_list.compare_exchange_weak(node->next_, node, std::memory_order_release)) {} // sched_yield
}

template<typename T>
void delete_later(T* data)
{
    add_to_delete_list(new data_to_delete(data));
}

void delete_no_hazard_nodes()
{
    data_to_delete* curr = delete_list.exchange(nullptr);

    while (curr != nullptr)
    {
        data_to_delete* const next = curr->next_;
        if (!is_hazard(curr->data_))
            delete curr;
        else
            add_to_delete_list(curr);

        curr = next;
    }
}

template<typename T>
struct node
{
    std::shared_ptr<T> data_;
    node<T>* next_;

    node(T const& data) : data_(std::make_shared<T>(data)) {}
};

template<typename T>
class lockfree_stack
{
private:

    std::atomic<node<T>*> head_;

public:

    void push(T const& data) {
        node<T>* new_node = new node<T>(data);
        new_node->next_ = head_.load(std::memory_order_acquire);
        while (!head_.compare_exchange_weak(new_node->next_, new_node, std::memory_order_release)) {}
    }

    std::shared_ptr<T> pop() {
        std::atomic<void*>& hazard_ptr = get_hazard_pointer_for_current_thread();
        node<T>* old_head = head_.load(std::memory_order_acquire);

        do {
            node<T>* temp;

            do {
                temp = old_head;
                hazard_ptr.store(old_head, std::memory_order_release);
                old_head = head_.load(std::memory_order_acquire);
            } while (temp != old_head);

        } while (old_head != nullptr && !head_.compare_exchange_weak(old_head, old_head->next_, std::memory_order_release));
        hazard_ptr.store(nullptr);

        std::shared_ptr<T> res;

        if (old_head != nullptr)
        {
            res.swap(old_head->data_);
            if (is_hazard(old_head))
                delete_later(old_head);
            else
                delete old_head;

            delete_no_hazard_nodes();
        }

        return res;
    }
};


void loop(lockfree_stack<int>& LFS, int mod) {

    if (mod == 1) {
        for (int i = 0; i < 50; ++i) {
            LFS.push(1);
            LFS.pop();
        }
    }
    if (mod == 2) {
        for (int i = 0; i < 100; ++i) LFS.push(1);
    }
    if (mod == 3) {
        for (int i = 0; i < 100; ++i) LFS.pop();
    }

    
}



int main()
{

    int THREADS = 6000;
    init_hazard_pointers_environment(THREADS);

    lockfree_stack<int> LFS;

    std::vector<std::thread> v_thread;

    int time_d = 0;
    int time_end = 0;
    int time_start = 0;

    std::cout << "Calculations begin" << std::endl;
    time_start = std::clock();



    for (int i = 0; i < THREADS; ++i) {
        v_thread.push_back(std::thread(&loop, std::ref(LFS), 2));
    }

    for (int i = 0; i < THREADS; ++i) {
        v_thread.at(i).join();
    }

    time_end = std::clock();
    time_d = time_end - time_start;
    std::cout << "Time push: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;



    v_thread.resize(0);

    for (int i = 0; i < THREADS; ++i) {
        v_thread.push_back(std::thread(&loop, std::ref(LFS), 3));
    }

    for (int i = 0; i < THREADS; ++i) {
        v_thread.at(i).join();
    }

    int time_end_2 = std::clock();
    time_d = time_end_2 - time_end;
    std::cout << "Time pop: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;




    v_thread.resize(0);

    for (int i = 0; i < THREADS; ++i) {
        v_thread.push_back(std::thread(&loop, std::ref(LFS), 1));
    }

    for (int i = 0; i < THREADS; ++i) {
        v_thread.at(i).join();
    }

    int time_end_3 = std::clock();
    time_d = time_end_3 - time_end_2;
    std::cout << "Time balance: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

    return 0;
}