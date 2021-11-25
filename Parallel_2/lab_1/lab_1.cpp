#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <mpi.h>
#include <ctime>
#include <iostream>

//#define VARIANT_0
//#define VARIANT_1B
#define VARIANT_2V

#define SAVE_DATA


#define ISIZE 10
#define JSIZE 10


void algorithm_0(int first_task, int last_task, double** a);
void algorithm_1B(int ProcRank, int ProcNum, int first_task, int last_task, double** a, double** a_copy);
void algorithm_2V(int ProcRank, int ProcNum, double** a, MPI_Status& stat);
void save(int ProcRank, int ProcNum, int first_task, int last_task, double** a, MPI_Status& stat);

int main(int argc, char** argv) {
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
    int task_per_proc = last_task - first_task;


    double* sending_data = new double[task_per_proc * JSIZE];

    for (int i = first_task; i < last_task; i++) {
        for (int j = 0; j < JSIZE; j++) {
            sending_data[(i - first_task) * JSIZE + j] = 10 * i + j;
        }
    }

    double* reciving_data = new double[JSIZE * ISIZE];

    if (ProcRank == 0) {
        int last_point = task_per_proc;

        for (int ii = 0; ii < task_per_proc * JSIZE; ++ii)
            reciving_data[ii] = sending_data[ii];

        for (int proc = 1; proc < ProcNum; ++proc) {
            int task_per_proc_rec;
            MPI_Recv(&task_per_proc_rec, sizeof(task_per_proc_rec), MPI_BYTE, proc, 1, MPI_COMM_WORLD, &stat);

            double* buff = new double[task_per_proc_rec * JSIZE];
            MPI_Recv(buff, task_per_proc_rec * JSIZE * sizeof(double), MPI_BYTE, proc, 1, MPI_COMM_WORLD, &stat);

            for (int ii = 0; ii < task_per_proc_rec * JSIZE; ++ii)
                reciving_data[last_point * JSIZE + ii] = buff[ii];

            last_point = last_point + task_per_proc_rec;
        }

        for (int ii = 1; ii < ProcNum; ++ii) {
            MPI_Send(reciving_data, JSIZE * ISIZE * sizeof(double), MPI_BYTE, ii, 1, MPI_COMM_WORLD);
        }
    }
    else {
        MPI_Send(&task_per_proc, sizeof(task_per_proc), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
        MPI_Send(sending_data, task_per_proc * JSIZE * sizeof(double), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

        MPI_Recv(reciving_data, JSIZE * ISIZE * sizeof(double), MPI_BYTE, 0, 1, MPI_COMM_WORLD, &stat);
    }

    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = reciving_data[i * JSIZE + j];
#ifdef VARIANT_1B
            a_copy[i][j] = a[i][j];
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
    algorithm_2V(ProcRank, ProcNum, a, stat);
#endif

    std::cout << "From process " << ProcRank << ": calculations completed." << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);


    if (ProcRank == 0) {

        time_end = std::clock();
        time_d = time_end - time_start;

        std::cout << "From process " << ProcRank << ": all calculations completed. Time " << double(time_d) / CLOCKS_PER_SEC << std::endl;
    }


#ifdef SAVE_DATA
        save(ProcRank, ProcNum, first_task, last_task, a, stat);
#endif //SAVE_DATA



    for (int i = 0; i < ISIZE; ++i) {
        delete[] a[i];
    }
    delete[] a;
#ifdef VARIANT_1B
    for (int i = 0; i < ISIZE; ++i) {
        delete[] a_copy[i];
    }
    delete[] a_copy;
#endif


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
void algorithm_2V(int ProcRank, int ProcNum, double** a, MPI_Status& stat) {
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
        int task_per_proc = last_task - first_task;

        double* sending_data = new double[task_per_proc];


        for (int c = first_task; c < last_task; c++) {
            int j = c % (JSIZE - 2);
            int shift = c / (JSIZE - 2);
            a[i + shift][j] = sin(0.00001 * a[i + shift - 3][j + 2]);
            sending_data[c - first_task] = a[i + shift][j];
        }

        double* reciving_data = new double[lines * ISIZE];

        if (ProcRank == 0) {
            int last_point = task_per_proc;

            for (int ii = 0; ii < task_per_proc; ++ii)
                reciving_data[ii] = sending_data[ii];
            
            for (int proc = 1; proc < ProcNum; ++proc) {
                int task_per_proc_rec;
                MPI_Recv(&task_per_proc_rec, sizeof(task_per_proc_rec), MPI_BYTE, proc, 1, MPI_COMM_WORLD, &stat);

                double* buff = new double[task_per_proc_rec];
                MPI_Recv(buff, task_per_proc_rec * sizeof(double), MPI_BYTE, proc, 1, MPI_COMM_WORLD, &stat);
           
                for (int ii = 0; ii < task_per_proc_rec; ++ii)
                    reciving_data[last_point + ii] = buff[ii];

                last_point = last_point + task_per_proc_rec;
            }

            for (int ii = 1; ii < ProcNum; ++ii) {
                MPI_Send(reciving_data, lines * ISIZE * sizeof(double), MPI_BYTE, ii, 1, MPI_COMM_WORLD);
            }



        }
        else {

            MPI_Send(&task_per_proc, sizeof(task_per_proc), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
            MPI_Send(sending_data, task_per_proc * sizeof(double), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

            MPI_Recv(reciving_data, lines * ISIZE * sizeof(double), MPI_BYTE, 0, 1, MPI_COMM_WORLD, &stat);

        }

        for (int c = 0; c < tasks_num; c++) {
            int j = c % (JSIZE - 2);
            int shift = c / (JSIZE - 2);
            a[i + shift][j] = reciving_data[c];
        }

        delete sending_data;
        delete reciving_data;
    }
}

#ifdef VARIANT_2V
void save(int ProcRank, int ProcNum, int first_task, int last_task, double** a, MPI_Status& stat) {

    if (ProcRank == 0) {
        FILE* ff;
        int flag = 0;

        ff = fopen("result.txt", "w");


        for (int i = 0; i < ISIZE; i++) {
            for (int j = 0; j < JSIZE; j++) {
                fprintf(ff, "%f ", a[i][j]);
            }
            fprintf(ff, "\n");
        }
        fclose(ff);

        std::cout << "From process " << ProcRank << ": data saved" << std::endl;
    }
}
#else 
void save(int ProcRank, int ProcNum, int first_task, int last_task, double ** a, MPI_Status& stat) {
    FILE* ff;
    int flag = 0;

    if (ProcRank != 0) {
        MPI_Recv(&flag, sizeof(flag), MPI_BYTE, ProcRank - 1, 1, MPI_COMM_WORLD, &stat);
        ff = fopen("result.txt", "a");
    }
    else {
        ff = fopen("result.txt", "w");
    }

    for (int i = first_task; i < last_task; i++) {
        for (int j = 0; j < JSIZE; j++) {
            fprintf(ff, "%f ", a[i][j]);
        }
        fprintf(ff, "\n");
    }
    fclose(ff);
    std::cout << "From process " << ProcRank << ": data saved" << std::endl;
    
    if (ProcRank != ProcNum - 1) {
        MPI_Send(&flag, sizeof(flag), MPI_BYTE, ProcRank + 1, 1, MPI_COMM_WORLD);
    }
}
#endif //VARIANT_2V
