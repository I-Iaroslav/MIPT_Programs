#include <dirent.h> 
#include <stdio.h> 


int main(void) {
    DIR *d;
    FILE *f;
    struct dirent *dir;

    
    char proc_dir[] = "/proc";

    d = opendir("/proc");

    printf("PID     CMD\n");

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] >= '0' && dir->d_name[0] <= '9') {

                char stat_way[11 + sizeof(dir->d_name)];
                snprintf(stat_way, sizeof(stat_way), "%s/%s/stat", proc_dir, dir->d_name);

                f = fopen(stat_way, "r");

                int pid;
                char proc_name[255];
                fscanf(f, "%d %s", &pid, proc_name);

                printf("%-8d%s\n", pid, proc_name);

                fclose(f);
            }
        }    
        closedir(d);
    }
    return(0);
}
