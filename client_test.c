#include "server.h"

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("Usage: client <time_out> <num_wanted_seats> <pref_seat_list>\n");
        return 1;
    }

    int time_out = atoi(argv[1]), num_wanted_seats = atoi(argv[2]);
    int pref_seat_list[512];

    for(int i = 3; i < argc; i++)
    {
    	pref_seat_list[i-3] = atoi(argv[i]);
    }

    pid_t pid = getpid();
    char *fifo_name = malloc(8),  *pid_c = malloc(5);
    strcat(fifo_name, "ans");

    sprintf(pid_c, "%d", (int) pid);
    strcat(fifo_name, pid_c);
    printf("%s\n", fifo_name);
    mkfifo(fifo_name, 770);

    int requests = open("requests", O_WRONLY);
    if(requests < 0)
    {
    	printf("error opening fifo 'requests'\n");
    	exit(1);
    }
    //TESTE
    write(requests,"12345 1 112 13 1313",19); //be careful of how many chars sent (calculate it beforehand)
    close(requests);

}
