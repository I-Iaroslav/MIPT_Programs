#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include "structs.h"


void print_data_block(char* buff, int buff_size);
void print_folder(struct directory_entry* DE);
void read_data_block(int fd, struct superblock* SB, int block_num, char* buff, int buff_size);
void read_directory(int fd, struct superblock* SB, int block_num);
void read_link_block(int fd, struct superblock* SB, int block_num, int level);
int sb_copy_check(int block_group, int inode_table);
void check(char* message);

int main() {

    int inode = 32001;

    struct superblock SB;
    struct block_group_descriptor_table BG;
    struct inode IN;


    char fs_name[] = "/home/yaroslav/ext2.img";
    int fd = open(fs_name, O_RDONLY);
    check("SB open");

    //читаем суперблок
    lseek(fd, 1024, SEEK_SET);
    check("SB lseek");
    read(fd, &SB, sizeof(SB));
    check("SB read");

    int block_group = (inode - 1) / SB.s_inodes_per_group;
    int local_inode_index = (inode - 1) % SB.s_inodes_per_group;

    int block_size = (1024 << SB.s_log_block_size);
    char buff[block_size]; 

    //читаем block group descriptor table
    //если размер блока = 1кб, то block group descriptor table находится в 3 блоке, иначе во 2
    if(block_size > 1024) {
        lseek(fd, block_size, SEEK_SET);
    }
    else {
        lseek(fd, block_size * 2, SEEK_SET);
    }
    check("BG lseek");
    read(fd, &BG, sizeof(BG));
    check("BG read");


    int shift = sb_copy_check(block_group, BG.bg_inode_table);


    //читаем нужную иноду
    lseek(fd, block_group * SB.s_blocks_per_group * block_size + block_size * shift + 128 * local_inode_index, SEEK_SET);  
    check("IN lseek");
    read(fd, &IN, sizeof(IN));
    check("IN read");


    //считываем датаблоки
    for(int i = 0; i < 15; ++i) {
        if(IN.i_block[i] == 0) { 
           i = 15; 
        }
        //dirrect block
        else if(i < 12) {
            if(IN.i_mode == I_MODE_FOLDER) { 
                read_directory(fd, &SB, IN.i_block[i]);
            }
            else { 
                read_data_block(fd, &SB, IN.i_block[i], buff, sizeof(buff));
                print_data_block(buff, sizeof(buff)); 
            }
        }
        //indirrect block
        else if(i == 12) {
            read_link_block(fd, &SB, IN.i_block[i], 1);
        }
        //doubly-inderrect block
        else if(i == 13) {
            read_link_block(fd, &SB, IN.i_block[i], 2);
        }
        //tryply-inderrect block
        else {
            read_link_block(fd, &SB, IN.i_block[i], 3);
        }
    }

    
    printf("\n");


}


void print_data_block(char* buff, int buff_size) {
    for(int i = 0; i < buff_size; ++i) {
        printf("%c", buff[i]);
    }
}

void print_folder(struct directory_entry* DE) {
    printf("inode_number: %d \n", DE->inode_number);
    printf("record_length: %d \n", DE->record_length);
    printf("name_length: %u \n", DE->name_length);
    printf("file_type: %u \n", DE->file_type);
    printf("name: ");
    for(int i = 0; i < DE->name_length; ++i) { 
        printf("%c", DE->name[i]);
    }
    printf("\n\n");
}


void read_directory(int fd, struct superblock* SB, int block_num) {
    int shift = 0;
    struct directory_entry DE;

    lseek(fd, block_num * (1024 << SB->s_log_block_size), SEEK_SET);
    check("read_directory lseek");

    
    while(shift < (1024 << SB->s_log_block_size) != 0) {
        read(fd, &DE, 8);
        check("read_directory read");
        read(fd, DE.name, DE.name_length);
        check("ead_directory read");

        shift += DE.record_length; 
        lseek(fd, block_num * (1024 << SB->s_log_block_size) + shift, SEEK_SET);
        check("read_directory lseek");

        print_folder(&DE);
    } 
}


void read_data_block(int fd, struct superblock* SB, int block_num, char* buff, int buff_size) {
    lseek(fd, block_num * (1024 << SB->s_log_block_size), SEEK_SET);
    check("read_data_block lseek");
    read(fd, buff, buff_size);
    check("read_data_block read");
}

void read_link_block(int fd, struct superblock* SB, int block_num, int level) {
    int link_size = (1024 << SB->s_log_block_size) / 4;
    int link[link_size];

    lseek(fd, block_num * link_size * 4, SEEK_SET);
    check("read_link_block lseek");
    read(fd, link, link_size);
    check("read_link_block read");


    for(int i = 0; i < link_size; ++i) {

        if(link[i] == 0) {
            i = link_size;
        }
        else if(level == 1) {
            char buff[link_size * 4];
            read_data_block(fd, SB, link[i], buff, sizeof(buff));
            print_data_block(buff, sizeof(buff));
        }
        else {
            read_link_block(fd, SB, link[i], level - 1);
        }
    }
}

//смотрим, есть ли в выбранной группе копия суперблока
//если нет, необходимо отступить 2 блока от начала группы(пропускаем block bitmap и inode bitmap) 
//иначе смотрим на значение начала inode table, взятого из block group descriptor table
int sb_copy_check(int block_group, int inode_table) {
    int shift = 2;
    if(block_group == 1 || block_group == 0) { shift = inode_table; }
    int x = block_group;
    while(x % 3 == 0) {
        x = x / 3;
        if(x == 1) { shift = inode_table; }
    }
    x = block_group;
    while(x % 5 == 0) {
        x = x / 3;
        if(x == 1) { shift = inode_table; }
    }
    x = block_group;
    while(x % 7 == 0) {
        x = x / 3;
        if(x == 1) { shift = inode_table; }
    }
    return shift;
}

void check(char* message) {
    if(errno != 0) {
        printf("Error! %s fail!\n", message);
        exit(1);
    }
}
