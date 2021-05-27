﻿#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <ctime>

#define L3_CACHE 12 //MB
#define BLOCK_SIZE L3_CACHE / 4 //MB
#define INT_PER_MB 262144 // 1
//1 MB = 1024 KB = 262144 INT = 512 * 512 INT

#define THREADS 3

#define MATRIX_A_HEIGHT 2000 
#define MATRIX_A_WIDTH 2300

#define MATRIX_B_HEIGHT 2300
#define MATRIX_B_WIDTH 2200

//#define MATRIX_A_HEIGHT 10 
//#define MATRIX_A_WIDTH 13
//
//#define MATRIX_B_HEIGHT 13
//#define MATRIX_B_WIDTH 9

template <typename T>
T** new_matrix(int height, int width) {
	T** matrix = new T * [height];
	for (int i = 0; i < height; ++i) {
		matrix[i] = new T[width];
	}
	return matrix;
}

template <typename T>
void delete_matrix(T** matrix, int height) {
	for (int i = 0; i < height; ++i) {
		delete[] matrix[i];
	}
	delete[] matrix;
}


void generate_matrix(int** matrix, int height, int width, bool zero) {
	if (zero == true) {
		for (int i = 0; i < height; ++i)
			for (int j = 0; j < width; ++j) matrix[i][j] = 0;
	}
	else {
		for (int i = 0; i < height; ++i)
			for (int j = 0; j < width; ++j) matrix[i][j] = (i + j) % 100;
	}
}

void block_matrix_value(std::pair<int, int>** block_matrix, int block_matrix_height, int block_matrix_width,
	int block_height, int block_width, int matrix_height, int matrix_width) {
	for (int i = 0; i < block_matrix_height - 1; ++i) {
		for (int j = 0; j < block_matrix_width - 1; ++j) {
			block_matrix[i][j] = std::pair<int, int>(block_height, block_width);
		}
		block_matrix[i][block_matrix_width - 1] = std::pair<int, int>(block_height, matrix_width - (block_matrix_width - 1) * block_width);
	}
	for (int k = 0; k < block_matrix_width - 1; ++k) {
		block_matrix[block_matrix_height - 1][k] = std::pair<int, int>(matrix_height - (block_matrix_height - 1) * block_height, block_width);
	}
	block_matrix[block_matrix_height - 1][block_matrix_width - 1] = std::pair<int, int>(matrix_height - (block_matrix_height - 1) * block_height, matrix_width - (block_matrix_width - 1) * block_width);
}


//принимает на вход матрицу А и транспонированную матрицу В
void matrix_multiply_thread(int** A, int A_height, int A_width, int** B, int B_height, int B_width, int** C, int num) {
	if (A_width != B_width) { return; }


	for (int ix = 0; ix < A_height; ++ix) {
		if (ix % THREADS == num) {

			for (int iy = 0; iy < B_height; ++iy) {
				for (int i = 0; i < A_width; ++i) {
					C[ix][iy] += A[ix][i] * B[iy][i];
				}
			}
		}
	}

}















int main() {



	if (MATRIX_A_WIDTH != MATRIX_B_HEIGHT) { std::cout << "Error 1" << std::endl; return 1; }
	int** matrix_A = new_matrix<int> (MATRIX_A_HEIGHT, MATRIX_A_WIDTH); 
	int** matrix_B = new_matrix<int> (MATRIX_B_HEIGHT, MATRIX_B_WIDTH);
	int** matrix_C = new_matrix<int> (MATRIX_A_HEIGHT, MATRIX_B_WIDTH);

	generate_matrix(matrix_A, MATRIX_A_HEIGHT, MATRIX_A_WIDTH, false); 
	generate_matrix(matrix_B, MATRIX_B_HEIGHT, MATRIX_B_WIDTH, false);
	generate_matrix(matrix_C, MATRIX_A_HEIGHT, MATRIX_B_WIDTH, true);


	//std::cout << std::endl << "A" << std::endl; 
	//for (int ix = 0; ix < MATRIX_A_HEIGHT; ++ix) {
	//	for (int iy = 0; iy < MATRIX_A_WIDTH; ++iy) {
	//		std::cout << matrix_A[ix][iy] << " ";
	//	}
	//std::cout << std::endl;
	//}
	//std::cout << std::endl << "B" << std::endl; 
	//for (int ix = 0; ix < MATRIX_B_HEIGHT; ++ix) {
	//	for (int iy = 0; iy < MATRIX_B_WIDTH; ++iy) {
	//		std::cout << matrix_B[ix][iy] << " ";
	//	}
	//	std::cout << std::endl;
	//}
	//std::cout << std::endl << "C" << std::endl; 
	//for (int ix = 0; ix < MATRIX_A_HEIGHT; ++ix) {
	//	for (int iy = 0; iy < MATRIX_B_WIDTH; ++iy) {
	//		std::cout << matrix_C[ix][iy] << " ";
	//	}
	//	std::cout << std::endl;
	//}


	int time_d = 0;
	int time_end = 0;
	int time_start = 0;

	std::cout << "Calculations begin" << std::endl;
	time_start = std::clock();


	//ширина и высота стандартного блока для матрицы А, для В их необходимо переставить местами 
	int block_A_width = std::sqrt(BLOCK_SIZE * INT_PER_MB); 
	int block_A_height = block_A_width + block_A_width % THREADS;

	int block_B_width = block_A_height; 
	int block_B_height = block_A_width;

	int add = 0;

	if (MATRIX_A_HEIGHT % block_A_height != 0) { add = 1; }
	else{ add = 0; }
	int block_matrix_A_height = MATRIX_A_HEIGHT / block_A_height + add;
	if (MATRIX_A_WIDTH% block_A_width != 0) {add = 1; }
	else{add = 0; }
	int block_matrix_A_width = MATRIX_A_WIDTH / block_A_width + add;
	if (MATRIX_B_HEIGHT % block_B_height != 0) { add = 1; }
	else{ add = 0; }
	int block_matrix_B_height = MATRIX_B_HEIGHT / block_B_height + add;
	if (MATRIX_B_WIDTH% block_B_width != 0) {add = 1; }
	else { add = 0; }
	int block_matrix_B_width = MATRIX_B_WIDTH / block_B_width + add;
	if (block_matrix_A_width != block_matrix_B_height) { std::cout << "Error 2" << std::endl; return 1; }

	//матрицы блоков
	std::pair<int, int>** block_matrix_A = new_matrix < std::pair<int, int>>(block_matrix_A_height, block_matrix_A_width);
	std::pair<int, int>** block_matrix_B = new_matrix < std::pair<int, int>>(block_matrix_B_height, block_matrix_B_width);

	//зададим значение высоты и ширины каждого блока
	block_matrix_value(block_matrix_A, block_matrix_A_height, block_matrix_A_width, block_A_height, block_A_width, MATRIX_A_HEIGHT, MATRIX_A_WIDTH);
	block_matrix_value(block_matrix_B, block_matrix_B_height, block_matrix_B_width, block_B_height, block_B_width, MATRIX_B_HEIGHT, MATRIX_B_WIDTH);
	//умножаем матрицы блоков как обычные матрицы
	for (int ix = 0; ix < block_matrix_A_height; ++ix) {
		for (int iy = 0; iy < block_matrix_B_width; ++iy) { 
			for (int i = 0; i < block_matrix_A_width; ++i) {
				//копируем блоки матрицы в отдельные массивы
				int** block_A = new_matrix<int>(block_matrix_A[ix][i].first, block_matrix_A[ix][i].second);

				//std::cout <<	1 << std::endl;
				for (int Ax = 0; Ax < block_matrix_A[ix][i].first; ++Ax) {
					for (int Ay = 0; Ay < block_matrix_A[ix][i].second; ++Ay) {
						block_A[Ax][Ay] = matrix_A[Ax + (ix * block_A_height)][Ay + (i * block_A_width)];
					}
				}

				//std::cout << 2 << std::endl;
				//копируем и сразу транспонируем
				int** block_B = new_matrix<int>(block_matrix_B[i][iy].second, block_matrix_B[i][iy].first);

				for (int Bx = 0; Bx < block_matrix_B[i][iy].first; ++Bx) {
					for (int By = 0; By < block_matrix_B[i][iy].second; ++By) {
						block_B[By][Bx] = matrix_B[Bx + (i * block_B_height)][By + (iy * block_B_width)];
					}
				}
				//создаем блок матрицы С и задаем его нулями
				int** block_C = new_matrix<int>(block_matrix_A[ix][iy].first, block_matrix_B[ix][iy].second);

				//std::cout << 5 << std::endi;
				generate_matrix(block_C, block_matrix_A[ix][iy].first, block_matrix_B[ix][iy].second, true);

				//std::cout << 6 << std::endl;

				std::vector<std::thread> v_thread; //вектор тредов

				//matrix_multiply_thread(std::ref(block_A), block_matrix_A[ix][i].first, block_matrix_A[ix][i].second,
				//	std::ref(block_B), block_matrix_B[i][iy].second, block_matrix_B[i][iy].first, std::ref(block_C), 0);

				for (int c = 0; c < THREADS; ++c) {
					v_thread.push_back(std::thread(&matrix_multiply_thread, block_A, block_matrix_A[ix][i].first, block_matrix_A[ix][i].second, 
						block_B, block_matrix_B[i][iy].second, block_matrix_B[i][iy].first, block_C, c));
				}
				for (int c = 0; c < THREADS; ++c) {
					v_thread.at(c).join();
				}






				//std::cout << 7 << Std::endi
				for (int Cx = 0; Cx < block_matrix_A[ix][iy].first; ++Cx) {
					for (int Cy = 0; Cy < block_matrix_B[ix][iy].second; ++Cy) {
						matrix_C[Cx + (ix * block_A_height)][Cy + (iy * block_B_width)] += block_C[Cx][Cy];
					}
				}

				delete_matrix<int>(block_A, block_matrix_A[ix][iy].first);
				delete_matrix<int>(block_B, block_matrix_B[ix][iy].second);
				delete_matrix<int>(block_C, block_matrix_A[ix][iy].first);

				//C [ix] [iy] += A[ix] [i] * В [iy] [i] ;
			}
		}
	}
	time_end = std::clock();
	time_d = time_end - time_start;
	std::cout << "Time: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

	//for (int ix = 0; ix < MATRIX_A_HEIGHT; ++ix) {
	//	for (int iy = 0; iy < MATRIX_B_WIDTH; ++iy) {
	//		std::cout << matrix_C[ix][iy] << " ";
	//	}
	//	std::cout << std::endl;
	//}



	delete_matrix<std::pair<int, int >> (block_matrix_A, block_matrix_A_height);
	delete_matrix<std::pair<int, int>>(block_matrix_B, block_matrix_B_height);
	delete_matrix<int> (matrix_A, MATRIX_A_HEIGHT);
	delete_matrix<int> (matrix_B, MATRIX_B_HEIGHT);
	delete_matrix<int> (matrix_C, MATRIX_A_HEIGHT);

	return 0;
}

	