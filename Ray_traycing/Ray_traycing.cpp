#include <iostream>
#include <mpi.h>
//#include "bmp.hpp"
#include "geometry.h"
#include "Moller_Trumbore.h"
#include <windows.h>
#include <vector>
#include <fstream>
#include <ctime>

#define SCREEN_SIZE_X 800
#define SCREEN_SIZE_Y 800



//#define SHOW_MOD //переключатель между программой трассировки и отрисовки

#ifndef SHOW_MOD 
int main(int argc, char* argv[]) {

	std::vector<tryangle> vtr; //вектор всех треугольников

	//считываем файл с треугольниками
	std::ifstream file("C:\\Programs\\MPI_Parallel\\MPI_RayTraycing\\MPI_RayTraycing\\MPI_RayTraycing\\model.xyz");
	float buff[9];
	
	if (!file) { std::cout << "File can't be open"; return 1; }

	for (int i = 0; i < 9; ++i) { file >> buff[i]; }
	while (!file.eof()) {
		vtr.push_back(tryangle(vector3D(buff[0], buff[1], buff[2]), vector3D(buff[3], buff[4], buff[5]), vector3D(buff[6], buff[7], buff[8])));
		for (int i = 0; i < 9; ++i) { file >> buff[i]; }
	}
	
	file.close();



	vector3D view(0,1,0); //точка, откуда смотрим
	vector3D dir(1,0,0); //куда смотрим

	vector3D light(1,10,10);
	//vector3D light(10,0,10);

	float dist = 0; //сюда будет записываться расстояние до пересечения с тругольником в алгоритме Мёллера
	int color = 245; 

	//говнокод с линалом
	vector3D dir_p(dir.x, dir.y, dir.z); //этим вектором потом будем пользоваться в цикле, он пробегает по каждому пикселю
	vector3D dir_n(dir.x, dir.y, dir.z);
	
	dir_n = dir_p / dir_p.norm();
	dir_p = dir_n;
	dir_p = dir_p - vector3D(-dir_n.y, dir_n.x, 0) / 2;
	dir_p = dir_p + vector3D(0, 0, 0.5);

	float dist_short = -1; //расстояние до треугольника, -1 - не пересекается ни с одним

	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = 0;

	int N = SCREEN_SIZE_X;
	int N_PerProcess, N_Add;
	int FirstX;

	time_start = std::clock();

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	//определяем, каким процессам достанется больше работы
	N_PerProcess = N / ProcNum;
	N_Add = N % ProcNum;
	
	if (ProcRank < N_Add) {
		N_PerProcess++;
		FirstX = N_PerProcess * ProcRank;
	}
	else {
		FirstX =  N_Add + N_PerProcess * ProcRank;
	}
	
	
	
	std::vector<std::vector<float>> screen_data; //сюда будем записывать яркость каждого пикселя

	dir_p = dir_p + vector3D(-dir_n.y, dir_n.x, 0) * FirstX / SCREEN_SIZE_X; //выставляем начальную точку по Х для каждого процесса

	for (int ix = FirstX; ix < FirstX + N_PerProcess; ++ix) {
		std::vector<float> column_data(SCREEN_SIZE_X); //сюда будем записывать яркость одного столбца
		column_data.insert(column_data.begin(), 0, SCREEN_SIZE_X); //выставляем яркость по умоляанию(0)

		for (int iy = 0; iy < SCREEN_SIZE_Y; ++iy) {
			for (tryangle i : vtr) {
	
				if (rayTriangleIntersect(view, dir_p, i, dist) == true) { //проверяем пересечение луча с каждым треугольником
					if (dist < dist_short || dist_short == -1) { //если втретили несколько подходящих, рисуем ближний
						vector3D norm = (i.Get3()-i.Get1()).cross((i.Get2()-i.Get1()));//вектор нормали треугольника
						norm = norm / norm.norm(); //норм
	
						vector3D light_dir = ((dir_p / dir_p.norm() )* dist) + (view - light);//вектор от источника света к точки пересечения с треугольником
						light_dir = light_dir / light_dir.norm();
	
						float c = light_dir.dot(norm);//проверяем угол между посчитанными выше векторами, с = [0, 1]
						if (c < 0) { c = 0; }//отбрасываем затененные (как и для обычных лучей, считается, что у треугольника одна сторона прозрачна)
						column_data[iy] = color * c + 10;//сохраняем яркость
	
						dist_short = dist;//сохроняем новое кратчайшее расстояние
					}
	
				}
			}
			dist_short = -1;
	
			dir_p = dir_p - vector3D(0, 0, 1) / SCREEN_SIZE_Y;//сдвигаем вектор
		}
		screen_data.push_back(column_data);
		column_data.clear();
		//сдвигаем вектор
		dir_p = dir_p + vector3D(0, 0, 1);
		dir_p = dir_p + vector3D(-dir_n.y, dir_n.x, 0) / SCREEN_SIZE_X;
	}
	
	if (ProcRank != 0) {
		std::cout << "From process " << ProcRank << ": job is done, sending the data " << std::endl;
		
		//начинаются танцы с бубном, вектор оказалось нельзя передовать, поэтому выделяется массив, через который все передаются в процесс 0
		float* sending_data = new float[N_PerProcess * SCREEN_SIZE_Y];
		int c = 0;
		for (std::vector<float> i : screen_data) {
			for (float ii : i) {
				sending_data[c] = ii;
				++c;
			}
		}
		MPI_Send(&N_PerProcess, sizeof(N_PerProcess), MPI_BYTE, 0, 1, MPI_COMM_WORLD); //у процессов мб разное колличество обрабатываемых столбцов, поэтому передаем их количество
		MPI_Send(sending_data, N_PerProcess * SCREEN_SIZE_Y * sizeof(float), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

		std::cout << "From process " << ProcRank << ": data sended succesfully " << std::endl;

		delete[] sending_data;
	}
	else {

		std::ofstream out;
		out.open("C:\\Programs\\MPI_Parallel\\MPI_RayTraycing\\MPI_RayTraycing\\MPI_RayTraycing\\out.txt");
		//записываем то, что насчитал процесс 0
		for (std::vector<float> i : screen_data) {
			for (float ii : i) {
				out << ii << " ";
			}
			out << "\n";
		}

		std::cout << "From process " << ProcRank << ": job is done, reciving the data " << std::endl;


		for (int i = 1; i < ProcNum; ++i) {
			//принимаем у каждого процесса данные
			int rec_N_PerProcess;//колличество обрабатываемых столбцов у процесса, который нам пересылает данные
			MPI_Recv(&rec_N_PerProcess, sizeof(rec_N_PerProcess), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			float* reciving_data = new float[rec_N_PerProcess * SCREEN_SIZE_Y];
			MPI_Recv(reciving_data, rec_N_PerProcess* SCREEN_SIZE_Y * sizeof(float), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			std::cout << "From process " << ProcRank << ": data recived from process " << i << std::endl;

			for (int ix = 0; ix < rec_N_PerProcess; ++ix) {
				for (int iy = 0; iy < SCREEN_SIZE_Y; ++iy) {
					out << reciving_data[ix * SCREEN_SIZE_X + iy] << " ";
				}
			}

			delete[] reciving_data;
		}

		out.close();

		time_end = std::clock();
		time_d = time_end - time_start;

		std::cout << "From process " << ProcRank << ": all job is done " << std::endl;
		std::cout << "Time: " << float(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;
	}

	MPI_Finalize();

	return 0;
}

#else
//вторая программа для отрисовки результата
int main(int argc, char* argv[]) {


	HDC hdc = GetDC(GetConsoleWindow());

	std::ifstream fin;
	fin.open("out.txt");
	float buff;

	for(int ix = 0 ; ix < SCREEN_SIZE_X; ++ix) {
		for (int iy = 0; iy < SCREEN_SIZE_Y; ++iy) {
			fin >> buff;
			SetPixel(hdc, ix, iy, RGB(buff, buff, buff));

		}
	}
	fin.close();

	SetPixel(hdc, 0, 0, RGB(255, 255, 255));
	SetPixel(hdc, SCREEN_SIZE_X, 0, RGB(255, 255, 255));
	SetPixel(hdc, 0, SCREEN_SIZE_Y, RGB(255, 255, 255));
	SetPixel(hdc, SCREEN_SIZE_X, SCREEN_SIZE_Y, RGB(255, 255, 255));

	while (true);
	return 0;
}
#endif