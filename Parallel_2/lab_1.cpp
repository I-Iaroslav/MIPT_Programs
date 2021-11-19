#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <mpi.h>
#include <ctime>
#include <iostream>

//#define VARIANT_0
//#define VARIANT_1B
#define VARIANT_2V

//#define SAVE_DATA


#define ISIZE 10000
#define JSIZE 10000


void algorithm_0(int first_task, int last_task, double** a);
void algorithm_1B(int ProcRank, int ProcNum, int first_task, int last_task, double** a, double** a_copy);
void algorithm_2V(int ProcRank, int ProcNum, double** a);

int main(int argc, char** argv) {
    FILE* ff;
    double** a = new double* [ISIZE];
    for (int i = 0; i < ISIZE; ++i) {
        a[i] = new double[JSIZE];
    }
#ifdef VARIANT_1B
    double** a_copy = new double* [ISIZE];       
    for (int i = 0; i < ISIZE; ++i) {
        a_copy[i] = new double[JSIZE];         
    }
#endif

    int ProcNum;
    int ProcRank;
    MPI_Status stat;

    int time_d = 0;
    int time_end = 0;
    int time_start = 0;


    int first_task, last_task;

    time_start = std::clock();

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);

    if (ProcRank == 0) {
        std::cout << "From process " << ProcRank << ": calculations begin" << std::endl;
    }


    //распределяем задачи
    first_task = (ISIZE / ProcNum) * ProcRank;
    if (ProcRank + 1 == ProcNum) { last_task = ISIZE; }
    else { last_task = (ISIZE / ProcNum) * (ProcRank + 1); }



    for (int i = first_task; i < last_task; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = 10 * i + j;
#ifdef VARIANT_1B
            a_copy[i][j] = 10 * i + j;
#endif
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);


#ifdef VARIANT_0
    algorithm_0(first_task, last_task, a);
#endif 
#ifdef VARIANT_1B
    algorithm_1B(ProcRank, ProcNum, first_task, last_task, a, a_copy);
#endif 
#ifdef VARIANT_2V
    algorithm_2V(ProcRank, ProcNum, a);
#endif //VARIANT



    std::cout << "From process " << ProcRank << ": calculations completed." << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    if (ProcRank == 0) {

        time_end = std::clock();
        time_d = time_end - time_start;

        std::cout << "From process " << ProcRank << ": all calculations completed. Time " << double(time_d) / CLOCKS_PER_SEC << std::endl;

#ifdef SAVE_DATA
        ff = fopen("result.txt", "w");
        for (int i = 0; i < ISIZE; i++) {
            for (int j = 0; j < JSIZE; j++) {
                fprintf(ff, "%f ", a[i][j]);
            }
            fprintf(ff, "\n");
        }
        fclose(ff);
        std::cout << "From process " << ProcRank << ": data saved" << std::endl;
#endif //SAVE_DATA

        for (int i = 0; i < ISIZE; ++i) {
            delete[] a[i];
        }
        delete[] a;
    }


    MPI_Finalize();

    return 0;
}




void algorithm_0(int first_task, int last_task, double** a) {
    for (int i = first_task; i < last_task; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = sin(0.00001 * a[i][j]);
        }
    }
}


//в этом варианте мы каждый раз ссылаемся на элемент, следующий после вычисляемого, следовательно нам нужна копия данных предыдущих вычислений
//так как иначе при расспаралеливании мы можем изменить эти данные раньше, чем мы их успеем использовать
void algorithm_1B(int ProcRank, int ProcNum,  int first_task, int last_task, double** a, double** a_copy) {

    if (ProcRank == ProcNum - 1) { last_task -= 3; }

    for (int i = first_task; i < last_task; i++) {
        for (int j = 4; j < JSIZE; j++) {
            a[i][j] = sin(0.00001 * a_copy[i + 3][j - 4]);      
        }
    }
}

//в этом варианте мы каждый раз ссылаемся на элемент, находящийся перед вычисляемым, следовательно нам нужна барьерная синхронизация
//так как иначе мы будем использовать еще не рассчитанные данные
void algorithm_2V(int ProcRank, int ProcNum, double** a) {
    //считаем 3 как один массив, и уже его параллелим между всеми исполнителями 
    for (int i = 3; i < ISIZE; i += 3) {
        int lines = 3;
        if (ISIZE - i < 3) { lines = ISIZE - i; }


        //распределяем задачи
        int tasks_num = (JSIZE - 2) * lines;
        int last_task, first_task;

        first_task = (tasks_num / ProcNum) * ProcRank;
        if (ProcRank + 1 == ProcNum) { last_task = tasks_num; }
        else { last_task = (tasks_num / ProcNum) * (ProcRank + 1); }

        for (int c = first_task; c < last_task; c++) {
            int j = c % (JSIZE - 2);
            int shift = c / (JSIZE - 2);
            a[i + shift][j] = sin(0.00001 * a[i + shift - 3][j + 2]);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

