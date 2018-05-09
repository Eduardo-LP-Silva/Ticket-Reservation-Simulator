#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99
#define WIDTH_PID 5
#define WIDTH_XXNN 5
#define WIDTH_SEAT 4
#define DELAY()

struct Seat* seats;
int requests;

struct Seat
{
	int numSeat;
	int clientId; //if client_pid < 0 -> seat is free
};

void *handleReservations(void *arg);
int isSeatFree(struct Seat *seats, int seatNum);
void bookSeat(struct Seat *seats, int seatNum, int clientId);
void freeSeat(struct Seat *seats, int seatNum);
struct Seat* createSeats(int numSeats);
