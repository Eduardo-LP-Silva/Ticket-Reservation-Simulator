#include "server.h"

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: client <time_out> <num_wanted_seats> <pref_seat_list>\n");
        return 1;
    }
    
    int time_out = atoi(argv[1]), num_wanted_seats = atoi(argv[2]);
    char *pref_seat_list = argv[3];

    pid_t pid = getpid();
    char *fifo_name = "ans", *pid_c = "";

    sprintf(pid_c, "%d", (int) pid);
    strcat(fifo_name, pid_c);
    mkfifo(fifo_name, 770);

    int requests = open("requests", O_WRONLY);


}