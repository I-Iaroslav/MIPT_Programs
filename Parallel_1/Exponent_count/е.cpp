#include <iostream>
#include <mpi.h>
#include <ctime>
#include <vector>
#include <fstream>
#include "VeryLongNumbers.hpp"

struct Message {
	VeryLongNumber last_fact;
	VeryLongNumber thread_sum;

	Message (VeryLongNumber fact, VeryLongNumber sum) : last_fact(fact), thread_sum(sum) {}
	Message () : last_fact(VeryLongNumber()), thread_sum(VeryLongNumber()) {}
	Message (const Message& M) {
		last_fact = M.last_fact;
		thread_sum = M.thread_sum;
	}

	void operator=(const Message& M) {
		last_fact = M.last_fact;
		thread_sum = M.thread_sum;
	}
};

int main(int argc, char* argv[]) {
	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = 0;
	
	int N = 450;
	int N_PerProcess, N_Add;
	int FirstTerm;

	int arr[1] = { 1 };
	VeryLongNumber fact(arr , 1), sum, k;

	time_start = std::clock();

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	N_PerProcess = N / ProcNum;
	N_Add = N % ProcNum;

	if (ProcRank < N_Add) {
		N_PerProcess++;
		FirstTerm = N_PerProcess * ProcRank + 1;
	}
	else {
		FirstTerm = (N_PerProcess + 1) * N_Add + N_PerProcess * (ProcRank - N_Add) + 1;
	}



	for (int i = 0; i < N_PerProcess; ++i) {
		fact /= i + FirstTerm;
		sum += fact;
	}



	if (ProcRank != 0) {
		Message M(fact, sum);
		std::cout << "From process " << ProcRank << ": sum from " << FirstTerm << " to " << FirstTerm + N_PerProcess - 1 << " is calculated " << std::endl;

		MPI_Send(&M, sizeof(M), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

	}
	else {
		std::cout << "From process " << ProcRank << ": sum from " << FirstTerm << " to " << FirstTerm + N_PerProcess - 1 << " is calculated " << std::endl;

		Message M;
		k = fact;

		for (int i = 1; i < ProcNum; ++i) {
			MPI_Recv(&M, sizeof(M), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);
			sum += (k * M.thread_sum);
			k *= M.last_fact;
		}

		sum += VeryLongNumber(arr, 1);

		time_end = clock();
		time_d = time_end - time_start;

		std::cout << "From process " << ProcRank << ": receive all" << std::endl
			<< "Result: " << sum << std::endl << "Time: " << float(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;



		std::ofstream fout("out.txt");
		fout << sum;
		fout.close();

	}

	MPI_Finalize();

	return 0;
}
