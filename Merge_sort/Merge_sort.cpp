#include<stdio.h>
#include<iostream>
#include<mpi.h>
#include<ctime>
#include<vector>
#include<iterator>
#include<fstream>

#define ARRAY_SIZE 1000000 //������ ������������ �������

//������� ��� ��������� ������ �������)
const std::string currentDateTime() {
	time_t     now = time(NULL);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%X", &tstruct);
	return buf;
}

//��������� ��������� ������
void generate_array(std::vector<int>& arr, int size) {
	srand(time(NULL));
	for (int i = 0; i < size; ++i) arr.push_back(rand());
}

void generate_good_array(std::vector<int>& arr, int size) {
	for (int i = 0; i < size - 1; ++i) arr.push_back(i);
	arr.push_back(size - 3);
}

void generate_bad_array(std::vector<int>& arr, int size) {
	for (int i = 0; i < size; ++i) arr.push_back(size - i);
}

//������� ������� ���� ��������������� ��������
void merge(std::vector<int>::iterator it_begin, std::vector<int>::iterator it_middle, std::vector<int>::iterator it_end) {
	std::vector<int> vec1;
	std::vector<int> vec2;

	//���������� � ������� ����� ��������� ��������
	for (std::vector<int>::iterator it = it_middle; it > it_begin ; --it) vec1.push_back(*(it - 1));
	for (std::vector<int>::iterator it = it_end; it > it_middle; --it) vec2.push_back(*(it-1));
	
	int buff1;
	int buff2;

	//������� �� ������ �������� �� ��������, �������� ����������
	for (std::vector<int>::iterator it = it_begin; it < it_end; ++it) {
		if (vec1.empty() == 1 || vec2.empty() == 1) {//������, ����� ���� �� �������� ����
			if (vec1.empty() == 1) {
				*it = vec2.back();
				vec2.pop_back();
			}
			else {
				*it = vec1.back();
				vec1.pop_back();
			}
		}
		else {
			buff1 = vec1.back();
			buff2 = vec2.back();
			if (buff1 > buff2) {
				*it = buff1;
				vec1.pop_back();
			}
			else {
				*it = buff2;
				vec2.pop_back();
			}
		}
	}

}

//���������� ��������
void mergesort(std::vector<int>::iterator it_begin, std::vector<int>::iterator it_end) {
	switch(it_end - it_begin) {//������� �� ������ ��������� �������
		case 0:
			return;
		case 1:
			break;
		case 2: 
			if (*it_begin < * (it_end - 1)) {
				int buff = *it_begin;
				*it_begin = *(it_end - 1);
				*(it_end - 1) = buff;
			}
			break;
		default: //���� ������ ������� ������, ��� �� 2 ���������, ���������� �������� ��������, ����� ���� ������� ��������������� �������
			std::vector<int>::iterator it_middle = it_begin + (it_end - it_begin) / 2;
			mergesort(it_begin, it_middle);
			mergesort(it_middle, it_end);
			merge(it_begin, it_middle, it_end);
	}
} 


int main(int argc, char* argv[]) {

	int ProcNum;
	int ProcRank;
	MPI_Status stat;
	int time_d = 0;
	int time_end = 0;
	int time_start = 0;


	std::vector<int> arr;
	generate_good_array(arr, ARRAY_SIZE);

	int N_PerProcess;
	int FirstX;

	time_start = std::clock();

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

	//������������ ������ ����� ������
	if (ProcRank != ProcNum - 1) {
		N_PerProcess = arr.size() / ProcNum;
	}
	else {
		N_PerProcess = arr.size() - ProcRank * (arr.size() / ProcNum);
	}
	FirstX = ProcRank * (arr.size() / ProcNum);

	//if (ProcRank == 0) {
		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations begin "<< N_PerProcess << std::endl;
	//}

	//�������� ����� �������, ������� ����� ������������ ����� ���������
	std::vector<int> proc_arr(N_PerProcess);
	std::copy(arr.begin() + FirstX, arr.begin() + FirstX + N_PerProcess, proc_arr.begin());
	
	//���������� ��������
	mergesort(proc_arr.begin(), proc_arr.end());

	if (ProcRank != 0) {
		//���������� ������, ����� ������� ������ ���������� � ������� 0
		int* sending_data = new int[N_PerProcess * sizeof(int)];
		int c = 0;
		for (int i : proc_arr) {
			sending_data[c] = i;
			++c;
		}
		MPI_Send(&N_PerProcess, sizeof(N_PerProcess), MPI_BYTE, 0, 1, MPI_COMM_WORLD); //� ��������� �� ������ ����� �������, ������� �������� �����
		MPI_Send(sending_data, N_PerProcess * sizeof(int), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data sended succesfully " << std::endl;

		delete[] sending_data;
	}
	else {
		int size = arr.size();
		arr.clear();//������� ����������� ������, ���� ����� ���������� ���������������

		//�������� ������ ��������, � ������� ����� ��������� ���������� ���������� ���� ��������� 
		std::vector<std::vector<int>> recv_arr(ProcNum); 
		recv_arr[0] = proc_arr; 

		for (int i = 1; i < ProcNum; ++i) {
			//��������� � ������� �������� ������
			int rec_N_PerProcess;//����� ������� � ��������, ������� ��� ���������� ������
			MPI_Recv(&rec_N_PerProcess, sizeof(rec_N_PerProcess), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			int* reciving_data = new int[rec_N_PerProcess];
			MPI_Recv(reciving_data, rec_N_PerProcess * sizeof(int), MPI_BYTE, i, 1, MPI_COMM_WORLD, &stat);

			for (int j = 0; j < rec_N_PerProcess; ++j) {
				recv_arr[i].push_back(reciving_data[j]);
			}

			delete[] reciving_data;

			std::cout << "[" << currentDateTime() << "] ";
			std::cout << "From process " << ProcRank << ": data recived from process " << i << std::endl;
		}

		//��������� ��������������� �������
		int it_min = 0; //��������� �� ������, �� ������� �� ����� ����������� ��������
		int min, buff;
		bool first = true; //����� ��� ����������� ������� ��������� ������� � ������ �������� �����
		
		//������� ���������������� ��������, ������� �� ����������� � ���, ��� ���������� �������� ����� ���������� ���������, � �� 2 ��� � ���������������� ����������
		for (int i = 0; i < size; ++i) {
			for (int it = 0; it < ProcNum; ++it) {
				if (recv_arr[it].empty() == false) {
					buff = recv_arr[it].back();
					if (first == true) { 
						min = buff; 
						it_min = it; 
					}
					else if(buff < min){
						min = buff;
						it_min = it;				
					}
					first = false;
				}
			}
			arr.push_back(min);
			if (recv_arr[it_min].empty() == false) recv_arr[it_min].pop_back();
			first = true;
		}


		time_end = std::clock();
		time_d = time_end - time_start;		

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": calculations completed. Time " << float(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

		//��������� ��������� � ����
		std::ofstream out;
		char file_name[] = "C:\\Programs\\MPI_Parallel\\Parallel_Sort\\out.txt";
		out.open(file_name);
		for (auto i : arr) out << i << " ";
		out.close();

		std::cout << "[" << currentDateTime() << "] ";
		std::cout << "From process " << ProcRank << ": data saved in file: " << file_name << std::endl;
	}

	MPI_Finalize();

	return 0;
}