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
    for(int n = strlen(pid_c); n < WIDTH_PID ; n++)
    	strcat(fifo_name, "0");
    strcat(fifo_name, pid_c);
    printf("%s\n",fifo_name);
    mkfifo(fifo_name,  0660);

    int requests = open("requests", O_WRONLY), response[argc - 2];
    char* request = malloc(512);


    for(int n = strlen(pid_c); n < WIDTH_PID ; n++)
       	strcat(request, "0");
    strcat(request, pid_c);
    for(int i = 2; i < argc; i++)
    {
    	strcat(request, " ");
    	strcat(request, argv[i]);
    }

    write(requests, request, strlen(request));

    clock_t start = clock();
    int answer = open(fifo_name, O_RDONLY);

    while((double) (clock() - start) / CLOCKS_PER_SEC <= time_out)
    {
        read(answer, response, (argc - 2)*sizeof(int));

        if(errno == EAGAIN)
        {
            errno = 0;
            continue;
        }
        else
            break;
    }
    close(answer);

    FILE *clog = fopen("clog.txt", "a");
    FILE *cbook = fopen("cbook.txt", "a");

    if((double) (clock() - start) / CLOCKS_PER_SEC > time_out)
    {
        printf("Client TimeOut\n");
        fprintf(clog, "%05d OUT\n", pid); //width pid
        return 1;
    }

    if(response[0] > 0)
        for(int i = 1; i < response[0]; i++)
            {
                fprintf(clog, "%05d %02d.%02d %04d\n", pid, i + 1, response[0], response[i]);
                fprintf(cbook, "%04d\n", response[i]);
            }
    else
    {
        fprintf(clog, "%05d ", pid);

        if(response[0] == -1)
            fprintf(clog, "MAX\n");
        else
            if(response[0] == -2)
                fprintf(clog, "NST\n");
            else
                if(response[0] == -3)
                    fprintf(clog, "IID\n");
                else
                    if(response[0] == -4)
                        fprintf(clog, "ERR\n");
                    else
                        if(response[0] == -5)
                            fprintf(clog, "NAV\n");
                        else
                            if(response[0] == -6)
                                fprintf(clog, "FUL\n");
    }
}
