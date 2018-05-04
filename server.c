#include "server.h"

void *reserve_seats(void *arg)
{

}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return -1;
    }

    int num_room_seats = atoi(argv[1]), num_ticket_offices = atoi(argv[2]), 
        open_time = atoi(argv[3]);

    mkfifo("requests", 770);

    int t, ticket_office;
    pthread_t tids[num_ticket_offices];

    for(t = 0; t < num_ticket_offices; t++)
        ticket_office = pthread_create(&tids[t], NULL, reserve_seats, NULL);

    int requests = open("requests", O_RDONLY);
    int *order[1];
    clock_t start;

    while((double) (clock() - start) / CLOCKS_PER_SEC <= open_time)
        read(requests, order[0], (MAX_CLI_SEATS + 2) * sizeof(int));

    for(t = 0; t < num_ticket_offices; t++)
        pthread_kill(tids[t], SIGINT);

    close(requests);
    
    unlink("requests");

    return 0;
}