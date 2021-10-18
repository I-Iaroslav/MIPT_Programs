#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

struct Superblock {
    int block_size;
    int inodes_per_group;
    int blocks_per_group;
} SB;

void print_data_block(char* buff, int buff_size) {
    for(int i = 0; i < buff_size; ++i) {
        printf("%c", buff[i]);
    }
}

void read_data_block(int fd, int block_num, char* buff, int buff_size) {
    lseek(fd, block_num * SB.block_size, SEEK_SET);
    read(fd, buff, buff_size);
}

void read_link_block(int fd, int block_num, int level) {
    char link_buff[4096];
    int link;
    read_data_block(fd, block_num, link_buff, sizeof(link_buff));
    for(int i = 0; i < 1024; ++i) {
        char link_c[4] = {link_buff[4*i], link_buff[4*i+1], link_buff[4*i+2], link_buff[4*i+3]};
        link = *(int*) link_c;
        //printf("\n%d, %d\n", link, i);
        if(link == 0) {}
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

    int inode = 32006;

    char buff[4096]; 

    char fs_name[] = "/home/yaroslav/ext2.img";
    int fd = open(fs_name, O_RDONLY);

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
    printf("block_group: %d, local_inode_index %d\n",block_group,  local_inode_index);


    //смотрим, есть ли в выбранной группе копия суперблока
    int shift = 0;
    int x = block_group;
    if(block_group == 1 || block_group == 0) { shift = 2; }
    while(x % 3 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }
    while(x % 5 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }
    while(x % 7 == 0) {
        x = x / 3;
        if(x == 1) { shift = 2; }
    }



    lseek(fd, block_group * SB.blocks_per_group * SB.block_size 
        + SB.block_size * (2 + shift) + 128 * local_inode_index 
        + 15 * SB.block_size , SEEK_SET);   //??????????

    read(fd, buff, sizeof(buff));


    int i_block[15];    
    for(int i = 0; i < 15;++i) {
        char buff_c[4] = {buff[40+i*4], buff[41+i*4], buff[42+i*4], buff[43+i*4]};
        i_block[i] = *(int*) buff_c;    
        //printf("i_block[%d]: %d \n", i, i_block[i]);
    }


    for(int i = 0; i < 15; ++i) {
        if(i_block[i] == 0) { 
            i = 15; 
        }
        //dirrect block
        else if(i < 12) {
            read_data_block(fd, i_block[i], buff, sizeof(buff));
            print_data_block(buff, sizeof(buff));
        }
        //indirrect block
        else if(i == 12) {
            read_link_block(fd, i_block[i], 1);
        }
        //doubly-inderrect block
        else if(i == 13) {
            read_link_block(fd, i_block[i], 2);
        }
        //tryply-inderrect block
        else {
            read_link_block(fd, i_block[i], 3);
        }
    }


    printf("\n");














    }
