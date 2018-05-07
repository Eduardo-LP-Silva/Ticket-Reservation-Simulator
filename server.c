#include "server.h"

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return -1;
    }

    int num_room_seats = atoi(argv[1]), num_ticket_offices = atoi(argv[2]), 
        open_time = atoi(argv[3]);

    if(num_room_seats < 0 || num_ticket_offices < 0 || open_time < 0)
    {
    	printf("Invalid arguments\n");
    	return -1;
    }

    //TESTES
/*
    printf("%d\n", isSeatFree(seats, 1));
    bookSeat(seats, 1, 235);
    printf("%d\n", isSeatFree(seats, 1));
    freeSeat(seats, 1);
    printf("%d\n", isSeatFree(seats, 1));*/

    mkfifo("requests", 770);

    int t;
    requests = open("requests", O_RDONLY);
    int order[1][MAX_CLI_SEATS];
    clock_t start = clock();

    //creating threads
    pthread_t tids[num_ticket_offices];
    seats = createSeats(num_room_seats);

    for (t = 0; t < num_ticket_offices; t++)
    {
    	int err = pthread_create(&tids[t], NULL, handleReservations, NULL);

    	if(err)
    	{
    		printf("Error creating thread");
    		exit(1);
    	}
    }

    while((double) (clock() - start) / CLOCKS_PER_SEC <= open_time)
        read(requests, order[0], (MAX_CLI_SEATS + 2) * sizeof(int));

    for(t = 0; t < num_ticket_offices; t++)
        pthread_kill(tids[t], SIGINT);

    close(requests);
    unlink("requests");
    free(seats);

    return 0;
}

struct Seat* createSeats(int numSeats)
{
	struct Seat* seats = malloc(numSeats*sizeof(struct Seat));

	for(int i = 0; i < numSeats; i++)
	{
		(seats+i*(sizeof(struct Seat)))->numSeat = i+1;
		(seats+i*(sizeof(struct Seat)))->clientId = -1;
	}
	return seats;
}

void *handleReservations(void *arg)
{
	//TODO: read from fifo; check if request is valid; if so, execute it
	return NULL;
}

int isSeatFree(struct Seat *seats, int seatNum)
{
	int isFree = 0;
	if((seats+(seatNum-1)*sizeof(struct Seat))->clientId < 0)
		isFree = 1;
	DELAY();
	return isFree;
}

void bookSeat(struct Seat *seats, int seatNum, int clientId)
{
	(seats+(seatNum-1)*sizeof(struct Seat))->clientId = clientId;
	DELAY();
}

void freeSeat(struct Seat *seats, int seatNum)
{
	(seats+(seatNum-1)*sizeof(struct Seat))->clientId = -1;
	DELAY();
}

