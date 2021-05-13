#include <iostream>
#include <mpi.h>


int main(int argc, char* argv[]) {
	int ProcNum;
	int ProcRank;
	MPI_Status stat;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	if (ProcRank == 0) {

		int data = 0;

		MPI_Send(&data, 1, MPI_INT, ProcRank + 1, 1, MPI_COMM_WORLD);

		std::cout << "From process " << ProcRank << ": send " << data << std::endl;

		MPI_Recv(&data, 1, MPI_INT, ProcNum - 1, 1, MPI_COMM_WORLD, &stat);

		std::cout << "From process " << ProcRank << ": receive " << data << std::endl;
	}
	else {

		int data;

		MPI_Recv(&data, 1, MPI_INT, ProcRank - 1, 1, MPI_COMM_WORLD, &stat);

		data += ProcRank;

		if (ProcRank == ProcNum - 1) { MPI_Send(&data, 1, MPI_INT, 0, 1, MPI_COMM_WORLD); }
		else { MPI_Send(&data, 1, MPI_INT, ProcRank + 1, 1, MPI_COMM_WORLD); }

		std::cout << "From process " << ProcRank << ": receive " << data - ProcRank << ", send " << data << std::endl;
	}

	MPI_Finalize();

	return 0;
}