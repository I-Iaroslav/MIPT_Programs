#include <iostream>
#include <sstream>
#include <fstream>
#include <omp.h>
#include <vector>

bool JOB_IS_DONE_FOR_ALL = false;

void print_field(int LEVEL, std::vector<std::vector<int>>& game_field) {
    int LEVEL_2 = LEVEL * LEVEL;
    for (int ix = 0; ix < LEVEL_2; ++ix) {
        for (int iy = 0; iy < LEVEL_2; ++iy) {
            if(game_field[ix][iy] > 9) {std::cout << game_field[ix][iy] << " ";}
            else { std::cout << game_field[ix][iy] << "  "; }

            if ((iy + 1) % LEVEL == 0 && iy != LEVEL_2 - 1) { std::cout << "|  "; }
        }
        std::cout << std::endl;

        if ((ix + 1) % LEVEL == 0 && ix != LEVEL_2 - 1) { 
            for (int i = 0; i < LEVEL_2 + LEVEL - 1; ++i) { std::cout << "_ _"; }
            std::cout << std::endl << std::endl;
        }
    }
}

//в файле первое значение - уровень судоку, после идут значения
void read_matrix(std::string file_name, int& LEVEL, std::vector<std::vector<int>>& game_field) {
    std::ifstream fin;
    std::string buf_str;
    int buf_int;

    fin.open(file_name);

    getline(fin, buf_str, ',');
    std::istringstream(buf_str) >> buf_int;

    LEVEL = buf_int;
    int LEVEL_2 = LEVEL * LEVEL;

    game_field.resize(LEVEL_2);
    for (int i = 0; i < LEVEL_2; ++i) { game_field[i].resize(LEVEL_2); }
    

    int i = 0;
    while (getline(fin, buf_str, ',')) {
        std::istringstream(buf_str) >> buf_int;
        game_field[i / LEVEL_2][i % LEVEL_2] = buf_int;
        ++i;
    }

    if (i != LEVEL_2 * LEVEL_2) {
        std::cout << "Error! Wrong file!" << std::endl;
        exit(1);
    }
}

//смотрим возможные для клетки значения
void find_possible_values(int LEVEL, std::vector<std::vector<int>>& game_field, std::vector<std::vector<bool>>& is_known,
    std::vector<bool>& possibles, int cell) {

    int LEVEL_2 = LEVEL * LEVEL;


    int cell_x = cell / LEVEL_2;
    int cell_y = cell % LEVEL_2;


    //смотрим значения по строке
    for (int iy = 0; iy < LEVEL_2; ++iy) {
        if (is_known[cell_x][iy] == true) {
            possibles[game_field[cell_x][iy]] = true;
        }
    }

    //смотрим значения по столбцу
    for (int ix = 0; ix < LEVEL_2; ++ix) {
        if (is_known[ix][cell_y] == true) {
            possibles[game_field[ix][cell_y]] = true;
        }
    }

    //смотрим значения по квадрату
    int start_x = (cell_x / LEVEL) * LEVEL;
    int start_y = (cell_y / LEVEL) * LEVEL;

    for (int ix = 0; ix < LEVEL; ++ix) {
        for (int iy = 0; iy < LEVEL; ++iy) {
            if (is_known[start_x + ix][start_y + iy] == true) {
                possibles[game_field[start_x + ix][start_y + iy]] = true;
            }
        }
    }
}


//возврощает 0 если решение найдено, 1 если решние невозможно
int solve_sudoku(int LEVEL, std::vector<std::vector<int>>& game_field) {

    bool JOB_IS_DONE = false;

    int LEVEL_2;
    int LEVEL_4;

    LEVEL_2 = LEVEL * LEVEL;
    LEVEL_4 = LEVEL_2 * LEVEL_2;

    std::vector<std::vector<bool>> is_known(LEVEL_2);   //матрица, указывающая на то, известно ли нам значение
                                                        //true - значение определенно, иначе false   

    std::vector<int> tasks;                             // стек заданий, в который помещаются клетки с неопределенным значением для данного раунда вычислений
    std::vector<int> next_tasks;                        // cтек заданий, в который помещаются клетки с неопределенным значением для следуещего раунда вычислений

    for (int i = 0; i < LEVEL_2; ++i) { is_known[i].resize(LEVEL_2); }

    
    //смотрим начальные значения клеток
    for (int ix = 0; ix < LEVEL_2; ++ix) {
        for (int iy = 0; iy < LEVEL_2; ++iy) {
            if (game_field[ix][iy] != 0) {
                is_known[ix][iy] = true;
            }
            else {
                is_known[ix][iy] = false;
                tasks.push_back(ix * LEVEL_2 + iy);
            }
        }
    }

    while (JOB_IS_DONE == false) {

        //вектор, в который записывается количество возможных значений в случае, когда значение 
        //однозначно не определяеся, иначе записывается невозможно большое значение
        std::vector<int> potential_vector(LEVEL_4, LEVEL_2 + 1); 
        

        for (int cell : tasks) {
            int cell_x = cell / LEVEL_2;
            int cell_y = cell % LEVEL_2;

            //вектор возможных значений
            //possibles[5] = true - значение 5 уже использовано и не может быть примененно к клетке 
            //possibles[0] - не используется           
            std::vector<bool> possibles(LEVEL_2 + 1, false);

            find_possible_values(LEVEL, game_field, is_known, possibles, cell);


            int first_false = 0;    //первое возможное значение
            int false_counter = 0;  //количество возможных в клетке значений
            for (int i = 1; i < LEVEL_2 + 1; ++i) {
                if (possibles[i] == false) {
                    false_counter++;
                    if (false_counter == 1) { first_false = i; }
                }
            }
            potential_vector[cell] = false_counter;

            //невозможно решить - убиваем поток
            if (false_counter == 0) { 
                return 1;
            }
            //найдено однозначное решение
            else if (false_counter == 1) {
                is_known[cell_x][cell_y] = true;
                game_field[cell_x][cell_y] = first_false;
            }
            //решеие однозначно не найдено
            else {
                next_tasks.push_back(cell);
            }

        }

        if (tasks.empty() == true) { JOB_IS_DONE = true; }

        //если мы не можем однозначно определить ни одно значение, рекурсивно брутфорсим судоку
        else if (tasks.size() == next_tasks.size()) {

            //ищем самую выгодную клетку для рекурсии
            std::pair<int, int> min_cell = std::pair<int, int>(-1, LEVEL_2 + 1); //<номер клетки, количество потенциальных значений>
            for (int i = 0; i < LEVEL_4; ++i) {
                if (potential_vector[i] < min_cell.second) { 
                    min_cell.first = i;
                    min_cell.second = potential_vector[i];
                    if(min_cell.second == 2) { i = LEVEL_4; }
                }
            }

            //находим еще раз потенциальные значения для выбранной клетки
            std::vector<bool> possibles(LEVEL_2 + 1, false);
            find_possible_values(LEVEL, game_field, is_known, possibles, min_cell.first);

            int result = 1;


            //пробегаем по всем возможным значениям
            for (int i = 1; i < LEVEL_2 + 1; ++i) {
                if (possibles[i] == false) {

//добовляем следующий блок в стек задач
#pragma omp task firstprivate(i, min_cell) shared(game_field, result)
{
                    //делаем копию поля и добавляем в нее предполагаемое значение выбранной клетки
                    std::vector<std::vector<int>> game_field_copy(LEVEL_2);
                    for (int i = 0; i < LEVEL_2; ++i) { game_field_copy[i].resize(LEVEL_2); }
                    game_field_copy = game_field;
                    game_field_copy[min_cell.first / LEVEL_2][min_cell.first % LEVEL_2] = i;
                    
                    if(JOB_IS_DONE == false) {
                        if (solve_sudoku(LEVEL, game_field_copy) == 0) { //решили
                            game_field = game_field_copy;
                            result = 0;
                        }
                    }
}
                }

            }
//ждем выполнения всех задач
#pragma omp taskwait

            //из блока omp task нельзя выйти с помощью return, поэтому используем костыль
            if(result == 0) { return 0; }
            //если все предположенные значения не привели к положительному результату, решения нет
            return 1;
        }
        tasks = next_tasks;
        next_tasks.clear();
    }
    return 0;
}


int main() {
    int LEVEL;                                                      // корень из стороны игрового поля (3 ~ 9 х 9, 5 ~ 25 х 25)
    int LEVEL_2;
    int LEVEL_4;


    std::vector<std::vector<int>> game_field;                       // матрица значений игрового поля, 0 - пустая клетка

    std::string file_name = "Sudoku_5_2.txt";

    read_matrix(file_name, LEVEL, game_field);
    LEVEL_2 = LEVEL * LEVEL;
    LEVEL_4 = LEVEL_2 * LEVEL_2;

    print_field(LEVEL, game_field);

    int time_d = 0;
    int time_end = 0;
    int time_start = 0;
	time_start = omp_get_wtime();

#pragma omp parallel
{
    #pragma omp single
    if (solve_sudoku(LEVEL, game_field) == 1) { 
        std::cout << "Error!" << std::endl;
    }
}

    time_end = omp_get_wtime();
    time_d = time_end - time_start;

    std::cout << std::endl;
    std::cout << std::endl;
    print_field(LEVEL, game_field);

    std::cout << std::endl << "Time: " << time_d << " seconds" << std::endl;
}