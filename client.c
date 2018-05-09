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

    int requests = open("requests", O_WRONLY), request[argc - 1], i, 
        answer = open(fifo_name, O_RDONLY | O_NONBLOCK), response[argc - 2];

    request[0] = pid;
    request[1] = num_wanted_seats;

    //j - Start, i - End
    int j, n;
    char buff[WIDTH_SEAT];

    for(i = 0, j = 0, n = 2; i < strlen(pref_seat_list); i++)
    {
        if(pref_seat_list[i] == ' ')
        {
            memcpy(buff, &pref_seat_list[j], i - j);
            buff[i - j] = '\0';
            request[n] = atoi(buff);

            j = i + 1;
            n++;
        }
    }

    write(requests, request, (argc - 1)*sizeof(int));

    clock_t start = clock();

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

    FILE *clog = fopen("clog.txt", "a");
    FILE *cbook = fopen("cbook.txt", "a");

    if((double) (clock() - start) / CLOCKS_PER_SEC > time_out)
    {
        printf("Client TimeOut\n");
        fprintf(clog, "%05d OUT\n", pid);
        return 1;
    }

    if(response[0] > 0)
        for(i = 1; i < response[0]; i++)
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