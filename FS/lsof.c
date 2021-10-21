#include <dirent.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <unistd.h> 
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>


void check(char* message) {
    if(errno != 0) {
        printf("Error! %s fail!\n", message);
        //exit(1);
    }
}


int main(void) {
    DIR *d;
    DIR *d_fd;
    FILE *f_stat;

    struct dirent *dir;
    struct dirent *dir_fd;

    
    char proc_dir[] = "/proc";

    d = opendir(proc_dir);
    check("opendir(/proc)");

    printf("PID      CMD              NAME\n");

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            check("readdir(d)");
            if(dir->d_name[0] >= '0' && dir->d_name[0] <= '9') {

                char stat_way[11 + sizeof(dir->d_name)];
                char fd_way[9 + sizeof(dir->d_name)];
                snprintf(stat_way, sizeof(stat_way), "%s/%s/stat", proc_dir, dir->d_name);
                snprintf(fd_way, sizeof(fd_way), "%s/%s/fd", proc_dir, dir->d_name);

                f_stat = fopen(stat_way, "r");
                check("fopen");

                int pid;
                char proc_name[255];
                fscanf(f_stat, "%d %s", &pid, proc_name);
                d_fd = opendir(fd_way);

                if(d_fd) {
                    while ((dir_fd = readdir(d_fd)) != NULL) {
                        check("opendir(d_fd)");
                        if(dir_fd->d_name[0] != '.') {
                            char buff[PATH_MAX];
                            char way[10 + sizeof(dir->d_name) + sizeof(dir_fd->d_name)];

                            snprintf(way, sizeof(way), "%s/%s", fd_way, dir_fd->d_name);
                            readlink(way, buff, sizeof(buff) - 1);
                            check("readlink");

                            printf("%-8d %-16s %-8s\n",pid, proc_name, buff);

                        }

                    }
                }
                else {
                    printf("Can not open %s\n", fd_way);
                }
                fclose(f_stat);
                closedir(d_fd);
                errno = 0;
            }
        }    
        closedir(d);
    }
    return(0);
}
