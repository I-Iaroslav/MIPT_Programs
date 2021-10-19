#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>

#define I_MODE_FOLDER 16893 //значение i_mode соответсвующее директории, найдено экспериментальным путем

struct Superblock {
    int block_size;
    int inodes_per_group;
    int blocks_per_group;
} SB;

struct Inode{
    int i_mode;
    int i_block[15];
} IN;

struct Directory_Entry {
    int inode_number;
    uint16_t record_length;
    uint8_t name_length;
    uint8_t file_type;
    char name[255];
};


void print_data_block(char* buff, int buff_size) {
    for(int i = 0; i < buff_size; ++i) {
        printf("%c", buff[i]);
    }
}

void print_folder(struct Directory_Entry DE) {
    printf("inode_number: %d \n", DE.inode_number);
    printf("record_length: %d \n", DE.record_length);
    printf("name_length: %u \n", DE.name_length);
    printf("file_type: %u \n", DE.file_type);
    printf("name: ");
    for(int i = 0; i < DE.name_length; ++i) { 
        printf("%c", DE.name[i]);
    }
    printf("\n\n");
}

void read_data_block_like_folder(char* buff, int buff_size) {
    int shift = 0;
    struct Directory_Entry DE;

    char inode_number_c[4] = {buff[0], buff[1], buff[2], buff[3]};
    DE.inode_number = *(int*) inode_number_c;

    while(shift < buff_size != 0) {

        char inode_number_c[4] = {buff[0 + shift], buff[1 + shift], buff[2 + shift], buff[3 + shift]};
        DE.inode_number = *(int*) inode_number_c;

        char record_length_c[2] = {buff[4 + shift], buff[5 + shift]};
        DE.record_length = *(uint16_t*) record_length_c;
            //printf("record_length: %u %u 0 0\n", buff[4 + shift], buff[5 + shift]);

        DE.name_length = buff[6 + shift];
        DE.file_type = buff[7 + shift];

        for(uint8_t i = 0;i < DE.name_length; ++i) {
            //printf("\n %u %c\n", i, buff[8 + i + shift]);
            DE.name[i] = buff[8 + i + shift];
        }

        shift += DE.record_length; 

        print_folder(DE);
    } 
}

void read_data_block(int fd, int block_num, char* buff, int buff_size) {
    lseek(fd, block_num * SB.block_size, SEEK_SET);
    read(fd, buff, buff_size);
}

void read_link_block(int fd, int block_num, int level) {
    char link_buff[SB.block_size];
    int link;

    read_data_block(fd, block_num, link_buff, sizeof(link_buff));

    for(int i = 0; i < SB.block_size / 4; ++i) {

        char link_c[4] = {link_buff[4*i], link_buff[4*i+1], link_buff[4*i+2], link_buff[4*i+3]};
        link = *(int*) link_c;

        if(link == 0) {
            i = SB.block_size / 4;
        }
        else if(level == 1) {
            char buff[4096];
            read_data_block(fd, link, buff, sizeof(buff));
            print_data_block(buff, sizeof(buff));
        }
        else {
            read_link_block(fd, link, level - 1);
        }
    }
}

int main() {

    int inode = 32001;

    char buff[4096]; 

    char fs_name[] = "/home/yaroslav/ext2.img";
    int fd = open(fs_name, O_RDONLY);

    //читаем суперблок
    lseek(fd, 1024, SEEK_SET);
    read(fd, buff, 1024);

    char block_size_c[4] = {buff[24], buff[25], buff[26], buff[27]};
    SB.block_size = 1024 << (*(int*) block_size_c);
    printf("Block Size: %d \n", SB.block_size);

    char inodes_per_group_c[4] = {buff[40], buff[41], buff[42], buff[43]};
    SB.inodes_per_group = *(int*) inodes_per_group_c;
    printf("inodes_per_group: %d \n", SB.inodes_per_group);

    char blocks_per_group_c[4] = {buff[32], buff[33], buff[34], buff[35]};
    SB.blocks_per_group = *(int*) blocks_per_group_c;
    printf("blocks_per_group: %d \n", SB.blocks_per_group);

    int block_group = (inode - 1) / SB.inodes_per_group;
    int local_inode_index = (inode - 1) % SB.inodes_per_group;
    printf("inode: %d, ", inode);
    printf("block_group: %d, local_inode_index %d\n\n",block_group,  local_inode_index);


    //смотрим, есть ли в выбранной группе копия суперблока
    int shift = 0;
    if(block_group == 1 || block_group == 0) { shift = 2; }
    int x = block_group;
    while(x % 3 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }
    x = block_group;
    while(x % 5 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }
    x = block_group;
    while(x % 7 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }


    //читаем нужную иноду
    lseek(fd, block_group * SB.blocks_per_group * SB.block_size 
        + SB.block_size * (2 + shift) + 128 * local_inode_index 
        + 15 * SB.block_size , SEEK_SET);   //??????????

    read(fd, buff, sizeof(buff));


    uint8_t i_mode_c[2] = {buff[0], buff[1]};
    IN.i_mode = *(uint16_t*) i_mode_c;

    for(int i = 0; i < 15;++i) {
        char buff_c[4] = {buff[40+i*4], buff[41+i*4], buff[42+i*4], buff[43+i*4]};
        IN.i_block[i] = *(int*) buff_c;    
        //printf("i_block[%d]: %d \n", i, i_block[i]);
    }


    //считываем датаблоки
    for(int i = 0; i < 15; ++i) {
        if(IN.i_block[i] == 0) { 
           i = 15; 
        }
        //dirrect block
        else if(i < 12) {
            read_data_block(fd, IN.i_block[i], buff, sizeof(buff));
            if(IN.i_mode == I_MODE_FOLDER) { 
                printf("This is folder:\n\n");
                read_data_block_like_folder(buff, sizeof(buff)); 
            }
            else { 
                printf("This is file:\n\n");
                print_data_block(buff, sizeof(buff)); 
            }
        }
        //indirrect block
        else if(i == 12) {
            read_link_block(fd, IN.i_block[i], 1);
        }
        //doubly-inderrect block
        else if(i == 13) {
            read_link_block(fd, IN.i_block[i], 2);
        }
        //tryply-inderrect block
        else {
            read_link_block(fd, IN.i_block[i], 3);
        }
    }

    
    printf("\n");


    }
