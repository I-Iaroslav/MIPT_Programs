#include <iostream>
#include <mpi.h>
#include <ctime>
//#include "VeryLongNumbers.hpp"

int main(int argc, char* argv[]) {
	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = std::clock();


	int N = 1000000000;
	int N_PerProcess, N_Add;
	int FirstTerm;
	double sum = 0;
	double recieve_sum = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	N_PerProcess = N / ProcNum;
	N_Add = N % ProcNum;

	if (ProcRank < N_Add) {
		N_PerProcess++;
		FirstTerm = N_PerProcess * (ProcRank + 1);
	}
	else {
		FirstTerm = (N_PerProcess + 1) * N_Add + N_PerProcess * (ProcRank - N_Add) + 1;
	}

	for (int i = 0; i < N_PerProcess; ++i) {
		sum += 1 / double(FirstTerm + i);
	}

	if (ProcRank != 0) {
		MPI_Send(&sum, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);

		std::cout << "From process " << ProcRank << ": sum from " << FirstTerm << " to " << FirstTerm + N_PerProcess - 1 << " is " << sum << std::endl;
	}
	else {
		std::cout << "From process " << ProcRank << ": sum from " << FirstTerm << " to " << FirstTerm + N_PerProcess - 1 << " is " << sum << std::endl;

		for (int i = 1; i < ProcNum; ++i) {
			MPI_Recv(&recieve_sum, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &stat);
			sum += recieve_sum;
		}

		time_end = clock();
		time_d = time_end - time_start;

		std::cout << "From process " << ProcRank << ": receive all" << std::endl 
			<< "Result: " << sum << " Time: " << float(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;
	}

	MPI_Finalize();

	return 0;
}
