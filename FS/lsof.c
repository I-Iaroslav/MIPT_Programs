#include <dirent.h> 
#include <stdio.h> 


int main(void) {
    DIR *d;
    DIR *d_fd;
    FILE *f_stat;

    struct dirent *dir;
    struct dirent *dir_fd;

    
    char proc_dir[] = "/proc";

    d = opendir("/proc");

    printf("PID     CMD\n");

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] >= '0' && dir->d_name[0] <= '9') {

                char stat_way[11 + sizeof(dir->d_name)];
                char fd_way[9 + sizeof(dir->d_name)];
                snprintf(stat_way, sizeof(stat_way), "%s/%s/stat", proc_dir, dir->d_name);
                snprintf(fd_way, sizeof(fd_way), "%s/%s/fd", proc_dir, dir->d_name);

                f_stat = fopen(stat_way, "r");

                int pid;
                char proc_name[255];
                fscanf(f_stat, "%d %s", &pid, proc_name);

                d_fd = opendir(fd_way);

                while ((dir_fd = readdir(d_fd)) != NULL) {
                    printf("%s\n", dir_fd->d_name);




                }

                fclose(f_stat);
                closedir(d_fd);

            }
        }    
        closedir(d);
    }
    return(0);
}
