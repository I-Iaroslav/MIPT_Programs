#include<stdio.h>
#include<iostream>
#include<mpi.h>
#include<ctime>
#include<vector>
#include<fstream>
#include <windows.h>


#define SIZE_X 1000
#define SIZE_Y 1000

//переключатель режимов вычисления и отображания
//#define SHOW_MODE

#ifndef SHOW_MODE
///////////////////////////////////////////////////////////////////////////////////////
//																					 //
// du(t, x)/dt + du(t, x)/dx = f(t, x),   0 <= t <= T,   0 <= x <= X;				 //
//																					 //
// u(0, x) = phi(x),   0 <= x <= X;													 //
//																					 //
// u(t, 0) = ksi(t),   0 <= t <= T;													 //
//																					 //
//																					 //
//	Крест:				Центральная трехточечная схема:		Левый уголок:			 //
//																					 //
//		 ^ solve()				  ^ solve_bottom()					^ solve_right()	 //
//		 |						  |									|				 //
// left  |     right			  |									|				 //
// <-----^----->			<----------->					<------->				 //
//       |central		    left  		right				left	central			 //
//		 |																			 //
//		 |																			 //
//	   bottom																		 //
///////////////////////////////////////////////////////////////////////////////////////


int N = SIZE_X; // по х
int K = SIZE_Y; // по t

//lineal
double tau = 0.01;
double h = 0.01;

//sin
//double tau = 0.01;
//double h = 0.01;

//exp
//double tau = 0.001;
//double h = 0.001;

double phi(double x) {
	//lineal
	return x;

	//exp
	//return exp(1);

	//sin
	//return 0;
}

double ksi(double t) {
	//lineal
	return t;

	//exp
	//return exp(1);

	//sin
	//return t * sin(t);
}

double f(double t, double x) {
	//lineal
	return 0;

	//exp
	//return exp(x * t / 2 + 1);

	//sin
	//return t * sin(x + t);
}

//Решение крестом
double solve(double left, double right, double bottom, double f) {
	return bottom + 2 * tau * (f + (left - right) / (2 * h));
}

//Решение явной центральной трехточечной схемой на нижней границе
double solve_bottom(double left, double right, double f) {
	return 0.5 * (left + right) + tau * (f + (left - right) / (2 * h));
}

//Решение уголком на правой границе
double solve_right(double left, double central, double f) {
	return central + tau * (f + (left - central) / h);
}

//отправкка данных
void data_send(double** data, MPI_Status& stat, int ProcNum, int ProcRank, int N_PerProcess, int k) {
	double send_right, send_left;

	if (ProcRank == 0) {
		send_right = data[k][N_PerProcess - 1];
		MPI_Send(&send_right, sizeof(send_right), MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	}
	else if (ProcRank == ProcNum - 1) {
		send_left = data[k][0];
		MPI_Send(&send_left, sizeof(send_left), MPI_BYTE, ProcRank - 1, 1, MPI_COMM_WORLD);
	}
	else {
		send_right = data[k][N_PerProcess - 1];
		MPI_Send(&send_right, sizeof(send_right), MPI_BYTE, ProcRank + 1, 1, MPI_COMM_WORLD);
		send_left = data[k][0];
		MPI_Send(&send_left, sizeof(send_left), MPI_BYTE, ProcRank - 1, 1, MPI_COMM_WORLD);
	}
}

//прием данных
std::pair<double, double> data_recive(double** data, MPI_Status& stat, int ProcNum, int ProcRank, int N_PerProcess, int k) {
	double recv_right, recv_left;

	if (ProcRank == 0) {
		MPI_Recv(&recv_right, sizeof(recv_right), MPI_BYTE, 1, 1, MPI_COMM_WORLD, &stat);
		recv_left = 0;
	}
	else if (ProcRank == ProcNum - 1) {
		MPI_Recv(&recv_left, sizeof(recv_left), MPI_BYTE, ProcRank - 1, 1, MPI_COMM_WORLD, &stat);
		recv_right = 0;
	}
	else {
		MPI_Recv(&recv_right, sizeof(recv_right), MPI_BYTE, ProcRank + 1, 1, MPI_COMM_WORLD, &stat);
		MPI_Recv(&recv_left, sizeof(recv_left), MPI_BYTE, ProcRank - 1, 1, MPI_COMM_WORLD, &stat);
	}

	return std::pair<double, double>(recv_left, recv_right);
}


//выделение памяти под матрицу
template <typename T>
T** new_matrix(int height, int width) {
	T** matrix = new T * [height];
	for (int i = 0; i < height; ++i) {
		matrix[i] = new T[width];
	}
	return matrix;
}

//удаление матрицы
template <typename T>
void delete_matrix(T** matrix, int height) {
	for (int i = 0; i < height; ++i) {
		delete[] matrix[i];
	}
	delete[] matrix;
}

//функция для красивого вывода времени)
const std::string currentDateTime() {
	time_t     now = time(NULL);
	struct tm  tstruct;
	char       buf[80];
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%X", &tstruct);
	return buf;
}





int main(int argc, char* argv[]) {

	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = 0;

	int N_PerProcess, N_Add;
	int FirstX;

	time_start = std::clock();

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	if (ProcRank == 0) {
		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations begin" << std::endl;
	}

	//определяем, каким процессам достанется больше работы
	N_PerProcess = N / ProcNum;
	N_Add = N % ProcNum;

	if (ProcRank < N_Add) {
		N_PerProcess++;
		FirstX = N_PerProcess * ProcRank;
	}
	else {
		FirstX = N_Add + N_PerProcess * ProcRank;
	}

	double** data = new_matrix<double>(K, N_PerProcess);

	//first line
	for (int n = 0; n < N_PerProcess; ++n) 	data[0][n] = phi((n + FirstX) * h);


	//second line
	//std::pair<double, double>	data_recv = data_exchange(data, stat, ProcNum, ProcRank, N_PerProcess, 0);

	//sending data
	data_send(data, stat, ProcNum, ProcRank, N_PerProcess, 0);

	//middle value
	for (int n = 1; n < N_PerProcess - 1; ++n) data[1][n] = solve_bottom(data[0][n - 1], data[0][n + 1], f(tau, (FirstX + n) * h));

	//reciving data
	std::pair<double, double> data_recv = data_recive(data, stat, ProcNum, ProcRank, N_PerProcess, 0);

	//first value
	if (ProcRank == 0)	data[1][0] = ksi(tau);
	else				data[1][0] = solve_bottom(data_recv.first, data[0][1], f(tau, FirstX * h));


	//last value
	if (ProcRank == ProcNum - 1)	data[1][N_PerProcess - 1] = solve_right(data[0][N_PerProcess - 2], data[0][N_PerProcess - 1], f(tau, h * (FirstX + N_PerProcess - 1)));
	else							data[1][N_PerProcess - 1] = solve_bottom(data[0][N_PerProcess - 2], data_recv.second, f(tau, h * (FirstX + N_PerProcess - 1)));



	//main part
	for (int k = 2; k < K; ++k) {
		//std::pair<double, double>	data_recv = data_exchange(data, stat, ProcNum, ProcRank, N_PerProcess, k - 1);

		//sending data
		data_send(data, stat, ProcNum, ProcRank, N_PerProcess, k - 1);

		//middle value
		for (int n = 1; n < N_PerProcess - 1; ++n) data[k][n] = solve(data[k - 1][n - 1], data[k - 2][n], data[k - 1][n + 1], f(k * tau, (FirstX + n) * h));

		//reciving data
		std::pair<double, double> data_recv = data_recive(data, stat, ProcNum, ProcRank, N_PerProcess, k - 1);

		//first value
		if (ProcRank == 0)	data[k][0] = ksi(k * tau);
		else				data[k][0] = solve(data_recv.first, data[k - 2][0], data[k - 1][1], f(k * tau, FirstX));

		//last value  
		if (ProcRank == ProcNum - 1)	data[k][N_PerProcess - 1] = solve_right(data[k - 1][N_PerProcess - 2], data[k - 1][N_PerProcess - 1], f(k * tau, (FirstX + N_PerProcess - 1) * h));
		else							data[k][N_PerProcess - 1] = solve(data[k - 1][N_PerProcess - 2], data[k - 2][N_PerProcess - 1], data_recv.second, f(k * tau, (FirstX + N_PerProcess - 1) * h));
	}


	//Сбор посчитанных данных в процесс 0
	if (ProcRank != 0) {
		double* reviving_data = new double[N_PerProcess * K];

		for (int iy = 0; iy < K; ++iy)
			for (int ix = 0; ix < N_PerProcess; ++ix)
				reviving_data[iy * N_PerProcess + ix] = data[iy][ix];


		MPI_Send(&N_PerProcess, sizeof(N_PerProcess), MPI_BYTE, 0, 1, MPI_COMM_WORLD); //у процессов мб разное колличество обрабатываемых столбцов, поэтому передаем их количество
		MPI_Send(&FirstX, sizeof(FirstX), MPI_BYTE, 0, 1, MPI_COMM_WORLD);//с какого элемента считал процесс
		MPI_Send(reviving_data, N_PerProcess * K * sizeof(double), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data sended to process 0 succesfully " << std::endl;

		delete[] reviving_data;
	}
	else {
		double** finale_data = new_matrix<double>(K, N);
		for (int iy = 0; iy < K; ++iy)
			for (int ix = 0; ix < N_PerProcess; ++ix)
				finale_data[iy][ix] = data[iy][ix];

		//принимаем у каждого процесса данные
		for (int i = 1; i < ProcNum; ++i) {
			int rec_N_PerProcess;//колличество обрабатываемых столбцов у процесса, который нам пересылает данные
			MPI_Recv(&rec_N_PerProcess, sizeof(rec_N_PerProcess), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			int proc_FirstX;//с какого элемента считал процесс
			MPI_Recv(&proc_FirstX, sizeof(proc_FirstX), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			double* reciving_data = new double[rec_N_PerProcess * K];
			MPI_Recv(reciving_data, rec_N_PerProcess * K * sizeof(double), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			std::cout << "[" << currentDateTime() << "] ";
			std::cout << "From process " << ProcRank << ": data recived from process  " << i << " succesfully" << std::endl;

			for (int iy = 0; iy < K; ++iy)
				for (int ix = 0; ix < rec_N_PerProcess; ++ix)
					finale_data[iy][ix + proc_FirstX] = reciving_data[iy * rec_N_PerProcess + ix];

		}

		time_end = std::clock();
		time_d = time_end - time_start;

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations completed. Time " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

		//сохраняем в файл
		std::ofstream out;
		char file_name[] = "C:\\Programs\\MPI_Parallel\\Lab1\\out.txt";
		out.open(file_name);
		for (int iy = 0; iy < K; ++iy) {
			for (int ix = 0; ix < N; ++ix) {
				out << finale_data[iy][ix] << " ";
			}
			out << std::endl;
		}
		out.close();
		
		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data saved in file: " << file_name << std::endl;

		delete_matrix<double>(finale_data, K);
	}


	delete_matrix<double>(data, K);

	MPI_Finalize();


	return 0;
}


#else
//вторая программа для отрисовки результата
int main(int argc, char* argv[]) {

	HDC hdc = GetDC(GetConsoleWindow());

	//lineal
	int norm = 10;

	//sin
	//int norm = 5000;

	//exp
	//int norm = 2000;

	std::ifstream fin;
	fin.open("C:\\Programs\\MPI_Parallel\\Lab1\\out.txt");
	double buff;

	for (int iy = 0; iy < SIZE_Y; ++iy) {
		for (int ix = 0; ix < SIZE_X; ++ix) {
			fin >> buff;
			SetPixel(hdc, iy, ix, RGB(255 * (buff / norm), 255 * (buff / norm), 255 * (buff / norm)));
		}
	}
	fin.close();

	while (true);
	return 0;
}
#endif