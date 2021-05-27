#include <atomic>
#include <thread>
#include <algorithm>
#include <vector>
#include <iostream>
#include <ctime>
#include <windows.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <exception>
#include <time.h>

#define kMaxIterations 100
#define ReverseSmoothingFactor 8



void BackOff_Strategy() {
	Sleep(0);
	//for (int i = 0; i < 1000; ++i);
}


class spinlock
{
public:

    virtual void Lock() = 0;
    virtual void Unlock() = 0;

};

class TAS_Spinlock : public spinlock{
public:
	void Lock() {
		while (lock_.exchange(true, std::memory_order_acquire)) {
			BackOff_Strategy();
		}
	}
	void Unlock() {
		lock_.store(false, std::memory_order_release);
	}
private:
	std::atomic<bool> lock_{ false };
};



class TTAS_Spinlock : public spinlock {
public:
	void Lock() {
		while (lock_.exchange(true, std::memory_order_acquire)) {
			do {
				BackOff_Strategy();
			} while (lock_.load(std::memory_order_relaxed));
		}
	}
	void Unlock() {
		lock_.store(false, std::memory_order_release);
	}
private:
	std::atomic<bool> lock_{ false };
};








#define NEW_THREAD 1
#define RELEASE 1

class Ticket_Lock : public spinlock {
public:
	void Lock() {
		auto ticket = ticket_.fetch_add(NEW_THREAD, std::memory_order_relaxed);
		while (owner_.load(std::memory_order_acquire) != ticket) {
			BackOff_Strategy();
		}
	}

	void Unlock() {
		owner_.fetch_add(RELEASE, std::memory_order_release);
	}

private:
	std::atomic<std::size_t> owner_{ 0 };
	std::atomic<std::size_t> ticket_{ 0 };
};



int THREADS = 500;


void loop(spinlock& SL, int& data, int& result, int& done) {
	for (int i = 0; i < 1000; ++i) {
		SL.Lock();
		data++;
		Sleep(0.01);
		if(i == 999) done++;
		SL.Unlock();
	}
	while (done != THREADS) Sleep(1);
	
	result = data;
}



int main() {
	TAS_Spinlock TAS;
	TTAS_Spinlock TTAS;
	Ticket_Lock TL;


	int data = 0;
	int done = 0;
	std::vector<std::thread> v_thread;
	std::vector<int> v_thread_result(THREADS); 

	int time_d = 0;
	int time_end = 0;
	int time_start = 0;

	std::cout << "Calculations begin" << std::endl;
	time_start = std::clock();



	for (int i = 0; i < THREADS; ++i) {
		v_thread.push_back(std::thread(&loop, std::ref(TTAS), std::ref(data), std::ref(v_thread_result.at(i)), std::ref(done)));
	}



	for (int i = 0; i < THREADS; ++i) {
		v_thread.at(i).join();
	}

	int result = v_thread_result.at(0);
	bool correct = true;
	for (int i = 0; i < THREADS; ++i) {
		if (v_thread_result.at(i) != result) correct = false;
	}
	if (correct == true) std::cout << result << std::endl;
	else std::cout << "Err" << std::endl;







	time_end = std::clock();
	time_d = time_end - time_start;
	std::cout << "Time: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;



	return 0;
}

