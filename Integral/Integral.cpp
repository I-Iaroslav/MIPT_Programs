#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <ctime>
#include "Semaphore.h"

#define TO_MANY_TASKS 7 //количество тасков, при котором последующий сбрасывается в общий стек

int THREADS = 4; // количество тредов

double e = 0.000001; //точность

//пределы интегрирования
double A = 0.01;
double B = 1;

//интегрируемая функция
double func(double x) {
    return sin(1 / (x * x));
}

//блоки, хранимые в стеках
struct Block {
    double a;
    double b;

    double f_a;
    double f_b;

    double integral; //итнтеграл блока, подсчитанный методом трапеций

    Block() : a(0), b(0), f_a(0), f_b(0), integral(0) {}
    Block(double new_a, double new_b, double new_f_a, double new_f_b) : a(new_a), b(new_b), f_a(new_f_a), f_b(new_f_b) {
        integral = ((new_f_a + new_f_b) / 2) * (new_b - new_a); //площадь трапеции
    }
    Block(const Block& copy) : a(copy.a), b(copy.b), f_a(copy.f_a), f_b(copy.f_b), integral(copy.integral) {}
    Block& operator=(const Block& Bl) {
        a = Bl.a;
        b = Bl.b;
        f_a = Bl.f_a;
        f_b = Bl.f_b;
        integral = Bl.integral;
        return *this;
    }
    bool operator==(const Block& Bl) {
        if (a == Bl.a &&
            b == Bl.b &&
            f_a == Bl.f_a &&
            f_b == Bl.f_b &&
            integral == Bl.integral)
            return true; 
        return false;
    }
} FakeBlock(0, -1, 0, 0); //используется для обозначения того, что задания кончились у всех процессов

std::ostream& operator<<(std::ostream& out, Block& Bl){
    out << "a = " << Bl.a << " b = " << Bl.b << " f_a = " << Bl.f_a << " f_b = " << Bl.f_b << std::endl;
    return out;
}


class Local_Stack {
private:
    std::vector<Block> v;
public:
    Block pop() {
        Block B = v.back();
        v.pop_back();
        return B;
    }
    void push(Block B) {
        v.push_back(B);
    }
    bool empty() {
        return v.empty();
    }
    int size() {
        return v.size();
    }
};


//глобальный стек с общей для все тредов памятью
class Global_Stack {
private:
    std::vector<Block> v;
    std::mutex mute;      //мьютекс, ограничевающий доступ к стеку
    Semaphore que;        //семафор, показывающий сколько заданий в стеке на момент обращения
    Semaphore im_working; //семафор, показывающий сколько процессов сейчас работают - 1
public:
    Global_Stack(int threads) : que(0), im_working(threads - 1) {}

    void push(Block Bl) {
        mute.lock();   //блокируем стек для остальных тредов
        v.push_back(Bl); 
        que.notify();  //увеличиваем семафор заданий
        mute.unlock(); //разблокируем стек
    }
    bool pop(Block& Bl, bool im_waiting) {
        if (!que.try_wait()) {  //смотрим, есть ли в стеке задания
            if (im_waiting) return false; //если нет и если тред в режиме ожидания, возвращаем false 
            else if (!im_working.try_wait()) { //иначе если тред только что закончил вычисления, пытаемся уменьшить семафор
                //если не получилось, значит все закончили работу и пора заканчивать работу
                Bl = FakeBlock; //блок, сигнализирующий о том, что все треды закончили работу
                mute.lock(); //блокируем стек для остальных тредов
                for (int i = 0; i < THREADS - 1; ++i) { //кладем столько фейковых блоков, сколько всего процессов - 1
                    v.push_back(FakeBlock);
                    que.notify(); //увеличиваем семафор заданий
                }
                mute.unlock(); //разблокируем стек
                return true;
            }
            return false;
        }
        //если есть, берем задание из стека
        if (im_waiting) im_working.notify(); //если тред в режиме ожидания, увеличиваем семафор
        mute.lock();   //блокируем стек для остальных тредов
        Bl = v.back(); 
        v.pop_back();
        mute.unlock(); //разблокируем стек
        return true;
    }
} GS(THREADS);


//функция треда
void loop(double start_a, double start_b, double &result, int pid) {
    double Local_Integral = 0;
    bool im_waiting = false;  //флаг, указывающий, находится ли процесс в ожидании задания из глобального стека
    bool job_is_done = false; //флаг, указывающий на завершение работы всех процессов
    int tasks = 0; //количество заданий в локальном стеке

    Local_Stack LS;
    LS.push(Block(start_a, start_b, func(start_a), func(start_b)));

    while (!job_is_done) {
        //берем задание из стека
        Block Bl(LS.pop());

        //делим блок на 2
        double c = (Bl.b + Bl.a) / 2;
        double f_c = func(c);

        Block Bl_left(Bl.a, c, Bl.f_a, f_c);
        Block Bl_right(c, Bl.b, f_c, Bl.f_b);

        double integral_2 = Bl_left.integral + Bl_right.integral;

        //если интеграл новых блоков достаточно точный, сохраняем результат
        if (abs(Bl.integral - integral_2) <= abs(e * integral_2)) {
            Local_Integral += integral_2;
        } //иначе добавляем 2 новых задания в стек
        else {
            LS.push(Bl_left);
            LS.push(Bl_right);
        }

        tasks = LS.size();
        //если заданий слишком много, сбрасываем их в глобальный стек
        if (tasks > TO_MANY_TASKS) {
            GS.push(LS.pop());            
        } //если заданий не осталось, берем из глобального стека
        else if (tasks == 0) {
            Block Bl_rec;
            //обращаемся к глобальному стеку, пока он не даст задания
            while (!GS.pop(Bl_rec, im_waiting)) {
                im_waiting = true;
                //sleep?
            }
            im_waiting = false;
            //если был получен FakeBlock, значит все треды остались без заданий и пора завершить процесс
            if (Bl_rec == FakeBlock) job_is_done = true;
            else LS.push(Bl_rec);
        }
    }

    result = Local_Integral;
}



int main()
{
    int time_d = 0;
    int time_end = 0;
    int time_start = 0;

    std::cout << "Calculations begin" << std::endl;
    time_start = std::clock();


    std::vector<std::thread> v_thread; //вектор тредов
    std::vector<double> v_result;      //вектор, куда треды будут класть результаты вычислений
    std::vector<double> v_start_value; //вектор значений отрезков для тредов

    double Integral = 0;


    for (int i = 0; i < THREADS; ++i) {
        v_result.push_back(0);
        v_start_value.push_back(A + ((B - A) / THREADS) * i);
    }
    v_start_value.push_back(B);

    //создаем нужное количество тредов
    for (int i = 0; i < THREADS; ++i) {
        v_thread.push_back(std::thread(&loop, v_start_value.at(i), v_start_value.at(i + 1), std::ref(v_result.at(i)), i + 1));
    }

    //ждем завершения работы всех тредов
    for (int i = 0; i < THREADS; ++i) {
        v_thread.at(i).join();
        Integral += v_result.at(i);
    }


    time_end = std::clock();
    time_d = time_end - time_start;
    std::cout << "Calculations completed. Answer: " << Integral << std::endl;
    std::cout << "Time: " << double(time_d) / CLOCKS_PER_SEC << " seconds" << std::endl;

    return 0;
}

