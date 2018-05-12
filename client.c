#include "server.h"

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("Usage: client <time_out> <num_wanted_seats> <pref_seat_list>\n");
        return 1;
    }
    int time_out = atoi(argv[1]), num_wanted_seats = atoi(argv[2]);

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
    char response1[512];


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
    int answer = open(fifo_name, O_RDONLY), answer_size;

    while((double) (clock() - start) / CLOCKS_PER_SEC <= time_out)
    {
        answer_size = read(answer, response1, (argc - 2)*sizeof(int));

        if(errno == EAGAIN)
        {
            errno = 0;
            continue;
        }
        else
            break;
    }
    close(answer);

    FILE *clog = fopen("clog.txt", "w");
    FILE *cbook = fopen("cbook.txt", "w");

    if((double) (clock() - start) / CLOCKS_PER_SEC > time_out)
    {
        printf("Client TimeOut\n");
        fprintf(clog, "%05d OUT\n", pid); //width pid
        return 1;
    }

    if(answer_size > 2)
    {
    	int num = 0, i = 0, j = 0;
		while (i < answer_size) {
			if (response1[i] != ' ')
				num = num * 10 + response1[i] - '0';
			else {
				response[j] = num;
				num = 0;
				j++;
			}
			i++;
		}
		response[j] = num;

		for (int i = 0; i < num_wanted_seats; i++) {
			fprintf(clog, "%05d %02d.%02d %04d\n", pid, i + 1, num_wanted_seats,
					response[i]);
			fprintf(cbook, "%04d\n", response[i]);
		}
    }
    else
    {
    	response[0] = -1*(response1[1]-'0');
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
