#include <iostream>
#include <mpi.h>
#include <Windows.h>
#include <ctime>
#include <fstream>

#define DEMONSTRATION_MODE

#ifdef DEMONSTRATION_MODE
#define SCREEN_HEIGHT  40 //40
#define SCREEN_WIDTH  60 //60
#else
#define SCREEN_HEIGHT  1000
#define SCREEN_WIDTH  1000
#endif

#define LIVE_TO_LIVE 3
#define LIVE_TO_DEATH_MAX 3
#define LIVE_TO_DEATH_MIN 2

#define TIME 4000 //количество итераций

//функция для красивого вывода времени)
const std::string currentDateTime() {
	time_t     now = time(NULL);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%X", &tstruct);
	return buf;
}

template <typename T>
T** new_matrix(int width, int height) {
	T** matrix = new T * [width];
	for (int i = 0; i < width; ++i) {
		matrix[i] = new T[height];
	}
	return matrix;
}

template <typename T>
void delete_matrix(T** matrix, int width) {
	for (int i = 0; i < width; ++i) {
		delete[] matrix[i];
	}
	delete[] matrix;
}

void print_map(bool ** map, int size_x, int size_y, HANDLE &hConsole, DWORD &dwBytesWritten) {
	wchar_t* screen = new wchar_t[2 * size_x * size_y + 1]; // Массив для записи в буфер
	
	for (int ix = 0; ix < size_x; ++ix)
		for (int iy = 0; iy < size_y; ++iy)
			if (map[ix][iy] == true) {
				screen[2 * (iy * size_x + ix)] = 0x2588;
				screen[2 * (iy * size_x + ix) + 1] = 0x2588;
			}
			else {
				screen[2 * (iy * size_x + ix)] = short(' ');
				screen[2 * (iy * size_x + ix) + 1] = short(' ');
			}
	
	screen[2 * size_x * size_y] = '\0';  // Последний символ - окончание строки
	WriteConsoleOutputCharacter(hConsole, screen, 2 * size_x * size_y, { 0, 0 }, &dwBytesWritten); // Запись в буфер
	
	delete[] screen;
}

void generate_with_zeros(bool** map, int x, int y) {
	for (int ix = 0; ix < x; ++ix) 
		for (int iy = 0; iy < y; ++iy) 
			map[ix][iy] = false;
}

void random_start_position(bool ** map) {
	srand(time(NULL));
	for (int ix = 0; ix < SCREEN_WIDTH; ++ix) {
		for (int iy = 0; iy < SCREEN_HEIGHT; ++iy) {
			map[ix][iy] = rand() % 2;
		}
	}
}

void start_position(bool** map) {
	for (int ix = 0; ix < SCREEN_WIDTH; ++ix) {
		for (int iy = 0; iy < SCREEN_HEIGHT; ++iy) {
			map[ix][iy] = false;
		}
	}
	//глайдер
	//map[3][3] = map[4][3] = map[5][3] = map[5][2] = map[4][1] = true; 
	//Gosper gun
	map[2][7] = map[2][8] = map[3][7] = map[3][8] = map[11][7] = map[12][6] = map[12][7] = map[12][8] = map[13][5] = map[13][6] = map[13][7] = map[13][8] = map[13][9] =
	map[14][4] = map[14][5] = map[14][9] = map[14][10] = map[18][6] = map[18][7] = map[18][8] = map[19][6] = map[19][7] = map[19][8] = map[21][5] = map[22][4] 
	= map[22][6] = map[23][3] = map[23][7] = map[24][4] = map[24][5] = map[24][6] = map[25][2] = map[25][3] = map[25][7] = map[25][8] = map[36][5] = map[36][6] 
	= map[37][5] = map[37][6] = true;
}





int main(int argc, char* argv[]) {

	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = 0;

	int column_per_process, column_add;
	int FirstX;

	bool** map = new_matrix<bool>(SCREEN_WIDTH, SCREEN_HEIGHT);
	start_position(map);

#ifdef DEMONSTRATION_MODE
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL); // Буфер экрана
	SetConsoleActiveScreenBuffer(hConsole); // Настройка консоли
	DWORD dwBytesWritten = 0;

	print_map(map, SCREEN_WIDTH, SCREEN_HEIGHT, hConsole, dwBytesWritten);
#endif
	time_start = std::clock();

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	//определяем, каким процессам достанется больше работы
	column_per_process = SCREEN_WIDTH / ProcNum;
	column_add = SCREEN_WIDTH % ProcNum;

	if (ProcRank < column_add) {
		column_per_process++;
		FirstX = column_per_process * ProcRank;
	}
	else {
		FirstX = column_add + column_per_process * ProcRank;
	}
#ifndef DEMONSTRATION_MODE
	if (ProcRank == 0) {
		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations begin" << std::endl;
	}
#endif
	bool** proc_map = new_matrix<bool>(column_per_process + 2, SCREEN_HEIGHT + 2);
	bool** new_map = new_matrix<bool>(column_per_process + 2, SCREEN_HEIGHT + 2);
	bool** visit_map = new_matrix<bool>(column_per_process + 2, SCREEN_HEIGHT + 2);

	generate_with_zeros(new_map, column_per_process + 2, SCREEN_HEIGHT + 2);
	generate_with_zeros(visit_map, column_per_process + 2, SCREEN_HEIGHT + 2);

	int proc_left = ProcRank - 1;
	int proc_right = ProcRank + 1;
	if (ProcRank == 0) proc_left = ProcNum - 1;
	if (ProcRank == ProcNum - 1) proc_right = 0;


	for (int ix = 0; ix < column_per_process + 2; ++ix) {
		for (int iy = 0; iy < SCREEN_HEIGHT + 2; ++iy) {
			if (ix == 0 || iy == 0 || ix == column_per_process + 1 || iy == SCREEN_HEIGHT + 1) {
				proc_map[ix][iy] = false;
			}
			else {
				proc_map[ix][iy] = map[FirstX + ix - 1][iy - 1];
			}
		}
	}



	for (int i = 0; i < TIME; ++i) {
		for (int j = 1; j < column_per_process + 1; ++j) {
			proc_map[j][0] = proc_map[j][SCREEN_HEIGHT];
			proc_map[j][SCREEN_HEIGHT + 1] = proc_map[j][1];
		}

#ifndef DEMONSTRATION_MODE
		bool* recive_array_left = new bool[SCREEN_HEIGHT + 2];
		bool* recive_array_right = new bool[SCREEN_HEIGHT + 2];
		for (int j = 0; j < SCREEN_HEIGHT + 2; ++j) {
			recive_array_left[j] = proc_map[1][j];
			recive_array_right[j] = proc_map[column_per_process][j];
		}

		MPI_Send(recive_array_left, (2 + SCREEN_HEIGHT) * sizeof(bool), MPI_BYTE, proc_left, 1, MPI_COMM_WORLD);
		MPI_Send(recive_array_right, (2 + SCREEN_HEIGHT) * sizeof(bool), MPI_BYTE, proc_right, 1, MPI_COMM_WORLD);

		MPI_Recv(recive_array_right, (2 + SCREEN_HEIGHT) * sizeof(bool), MPI_BYTE, proc_right, 1, MPI_COMM_WORLD, &stat);
		MPI_Recv(recive_array_left, (2 + SCREEN_HEIGHT) * sizeof(bool), MPI_BYTE, proc_left, 1, MPI_COMM_WORLD, &stat);

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << " circle sending is complete" << std::endl;

		for (int j = 0; j < SCREEN_HEIGHT + 2; ++j) {
			proc_map[0][j] = recive_array_left[j];
			proc_map[column_per_process + 1][j] = recive_array_right[j];
		}
#else
		for (int j = 0; j < SCREEN_HEIGHT + 2; ++j) {
			proc_map[0][j] = proc_map[column_per_process][j];
			proc_map[column_per_process + 1][j] = proc_map[1][j];
		}
#endif


		for (int ix = 1; ix < column_per_process + 1; ++ix) {
			for (int iy = 1; iy < SCREEN_HEIGHT + 1; ++iy) {
				if (proc_map[ix][iy] == true || ix == 1 || ix == column_per_process || iy == 1 || iy == SCREEN_HEIGHT) {
					for (int ax = -1; ax < 2; ++ax) {
						for (int ay = -1; ay < 2; ++ay) {
							if ((ix + ax != 0 && iy + ay != 0 && ix + ax != column_per_process + 1 && iy + ay != SCREEN_HEIGHT + 1) && visit_map[ix + ax][iy + ay] == false) {
								
								visit_map[ix + ax][iy + ay] = true;

								int c = 0;
								for (int kx = -1; kx < 2; ++kx)
									for (int ky = -1; ky < 2; ++ky)
										if (proc_map[ix + ax + kx][iy + ay + ky] == true && (kx != 0 || ky != 0)) c++;

								if (proc_map[ix + ax][iy + ay] == true) {
									if (c >= LIVE_TO_DEATH_MIN && c <= LIVE_TO_DEATH_MAX) new_map[ix + ax][iy + ay] = true;
									else new_map[ix + ax][iy + ay] = false;
								}
								else {
									if (c == LIVE_TO_LIVE) new_map[ix + ax][iy + ay] = true;
									else new_map[ix + ax][iy + ay] = false;
								}
							}
						}
					}
				}
			}
		}


		for (int ix = 1; ix < column_per_process + 1; ++ix) {
			for (int iy = 1; iy < SCREEN_HEIGHT + 1; ++iy) {
				proc_map[ix][iy] = new_map[ix][iy];
				map[ix - 1][iy - 1] = new_map[ix][iy];
			}
		}

		generate_with_zeros(new_map, column_per_process + 2, SCREEN_HEIGHT + 2);
		generate_with_zeros(visit_map, column_per_process + 2, SCREEN_HEIGHT + 2);


#ifndef DEMONSTRATION_MODE
		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": step " << i << " is calculated" << std::endl;
#else
		Sleep(100);
		print_map(map, SCREEN_WIDTH, SCREEN_HEIGHT, hConsole, dwBytesWritten);
#endif

	}


#ifndef DEMONSTRATION_MODE
	if (ProcRank != 0) {
		bool* reviving_data = new bool[column_per_process * SCREEN_HEIGHT];

		for (int iy = 0; iy < SCREEN_HEIGHT; ++iy)
			for (int ix = 0; ix < column_per_process; ++ix)
				reviving_data[iy * column_per_process + ix] = proc_map[ix + 1][iy + 1];


		MPI_Send(&column_per_process, sizeof(column_per_process), MPI_BYTE, 0, 1, MPI_COMM_WORLD); //у процессов мб разное колличество обрабатываемых столбцов, поэтому передаем их количество
		MPI_Send(&FirstX, sizeof(FirstX), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
		MPI_Send(reviving_data, column_per_process *  SCREEN_HEIGHT * sizeof(bool), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data sended to process 0 succesfully " << std::endl;
	}
	else {
		for (int ix = 0; ix < column_per_process; ++ix)
			for (int iy = 0; iy < SCREEN_HEIGHT; ++iy)
				map[ix][iy] = proc_map[ix + 1][iy + 1];


		for (int i = 1; i < ProcNum; ++i) {
			//принимаем у каждого процесса данные
			int rec_column_PerProcess;//колличество обрабатываемых столбцов у процесса, который нам пересылает данные
			MPI_Recv(&rec_column_PerProcess, sizeof(rec_column_PerProcess), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			int proc_FirstX;
			MPI_Recv(&proc_FirstX, sizeof(proc_FirstX), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			bool* reciving_data = new bool[rec_column_PerProcess *  SCREEN_HEIGHT];
			MPI_Recv(reciving_data, rec_column_PerProcess *  SCREEN_HEIGHT * sizeof(bool), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			std::cout << "[" << currentDateTime() << "] ";
			std::cout << "From process " << ProcRank << ": data recived from process  " << i  << " succesfully" << std::endl;

			for (int iy = 0; iy < SCREEN_HEIGHT; ++iy)
				for (int ix = proc_FirstX; ix < proc_FirstX + rec_column_PerProcess; ++ix)
					map[ix][iy] = reciving_data[iy * rec_column_PerProcess + (ix - proc_FirstX)];

		}


		time_end = std::clock();
		time_d = time_end - time_start;

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations completed. Time " << float(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

		std::ofstream out;
		char file_name[] = "C:\\Programs\\MPI_Parallel\\Parallel_live\\out.txt";
		out.open(file_name);
		for (int iy = 0; iy < SCREEN_HEIGHT; ++iy) {
			for (int ix = 0; ix < SCREEN_WIDTH; ++ix) {
				out << map[ix][iy] << " ";
			}
			out << std::endl;
		}
		out.close();

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data saved in file: " << file_name << std::endl;
	}
#endif

	delete_matrix<bool>(proc_map, column_per_process + 2);
	delete_matrix<bool>(new_map, column_per_process + 2);
	delete_matrix<bool>(visit_map, column_per_process + 2);
	
	MPI_Finalize();

	delete_matrix<bool>(map, SCREEN_WIDTH);

#ifdef DEMONSTRATION_MODE
	while (1);
#endif
	return 0;
}