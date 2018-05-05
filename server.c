#include "server.h"

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return -1;
    }

    int num_room_seats = atoi(argv[1]), num_ticket_offices = atoi(argv[2]), 
        open_time = atoi(argv[3]), *requests; //TODO: initialize requests!!

    if(num_room_seats < 0 || num_ticket_offices < 0 || open_time < 0)
    {
    	printf("Invalid arguments\n");
    	return -1;
    }

    struct Seat* seats = createSeats(num_room_seats);

    //TESTES
/*
    printf("%d\n", isSeatFree(seats, 1));
    bookSeat(seats, 1, 235);
    printf("%d\n", isSeatFree(seats, 1));
    freeSeat(seats, 1);
    printf("%d\n", isSeatFree(seats, 1));*/

    mkfifo("requests", 770);

    //creating threads
    pthread_t tid[num_ticket_offices];

    for(int i = 0; i < num_ticket_offices; i++)
    {
    	int err = pthread_create(&tid[i], NULL, handleReservations, requests);
    	if(err)
    	{
    		printf("Error creating thread");
    		exit(1);
    	}
    }

    unlink("requests");
    free(seats);

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

