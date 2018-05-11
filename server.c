#include "server.h"

int flag = 0;

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return -1;
    }

    int num_room_seats = atoi(argv[1]), num_ticket_offices = atoi(argv[2]), 
        open_time = atoi(argv[3]);
    numRoomSeats = atoi(argv[1]);

    if(num_room_seats < 0 || num_ticket_offices < 0 || open_time < 0)
    {
    	printf("Invalid arguments\n");
    	return -1;
    }

    seats = createSeats(num_room_seats);

    fdslog = open("slog.txt", O_WRONLY | O_CREAT | O_TRUNC, 0750);

    remove("requests");
    if(mkfifo("requests", 0660) < 0)
    {
    	printf("error making fifo requests\n");
    	exit(2);
    }

    int t;
    requests = open("requests", O_RDONLY);
    if(requests<0)
    {
    	printf("error opening fifo 'requests'\n");
    	exit(1);
    }

    char reservations[MAX_CLI_SEATS];
    clock_t start = clock();

    //creating threads
    pthread_t tids[num_ticket_offices];

    for (t = 0; t < num_ticket_offices; t++)
    {
    	int err = pthread_create(&tids[t], NULL, handleReservations, (void*) t+1);

    	if(err)
    	{
    		printf("Error creating thread");
    		exit(3);
    	}
    }

   while((double) (clock() - start) / CLOCKS_PER_SEC <= open_time)
   {
    	int numRead = read(requests, reservations, (MAX_CLI_SEATS + 2) * sizeof(int));
    	int i = 0, j = 0;
        int num = 0;
        while(i < numRead){
        	if(reservations[i] != ' ')
        		num = num*10 + reservations[i]-'0';
        	else
        	{
        		order[0][j] = num;
        		num = 0;
        		j++;
        	}
        	i++;
        }
        order[0][j] = num;
        order_size = j+1;
    }

    for(t = 0; t < num_ticket_offices; t++)
    {
    	flag = 1;
    	writeOpenCloseLogFile(t+1, 0);
    }
    writeBookingsFile();
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

void writeOpenCloseLogFile(int thread, int open)
{
	char* output = malloc(512);
	snprintf(output, 3, "%d", thread);
	if (strlen(output) == 1) {
		write(fdslog, "0", 1);
	}
	write(fdslog, output, 2);
	if(open)
		write(fdslog, "-OPEN\n", 6);
	else
		write(fdslog,"-CLOSE\n",7);
}

void writeBookingsFile()
{
	int fdbook = open("sbook.txt", O_WRONLY | O_CREAT | O_TRUNC, 0750);

	for(int i = 1; i <= numRoomSeats; i++)
	{
		if(!isSeatFree(seats, i))
		{
			char* output = malloc(512);
			snprintf(output, 10, "%d",  i);
			for(int n = strlen(output); n < WIDTH_SEAT; n++)
				write(fdbook, "0", 1);
			write(fdbook,output,10);
			write(fdbook, "\n", 1);
			free(output);
		}
	}
	close(fdbook);
}

void *handleReservations(void *arg)
{
	pthread_mutex_lock(&mut);
	int thread = (int) arg;
	writeOpenCloseLogFile(thread, 1);
	pthread_mutex_unlock(&mut);

	while(1)
	{
		pthread_mutex_lock(&mut);
		if(order[0][0] == 0)
		{
			pthread_mutex_unlock(&mut);
			continue;
		}
		int o_size = order_size;
		int order1[o_size];
		for(int i = 0; i < o_size; i++)
		{
			order1[i] = order[0][i];
			order[0][i] = 0;
		}
		pthread_mutex_unlock(&mut);

		int client_pid = order1[0];
		char* fifo_name = malloc(8* sizeof(char)), *pid_c = malloc(WIDTH_PID*sizeof(char));
		strcat(fifo_name, "ans");
		snprintf(pid_c, WIDTH_PID+1, "%d", client_pid);
		for(int n = strlen(pid_c); n < WIDTH_PID; n++)
			strcat(fifo_name, "0");
		strcat(fifo_name, pid_c);
		int fifo = open(fifo_name, O_WRONLY);
		if(fifo < 0)
		{
			printf("error opening fifo: %s\n", fifo_name);
			exit(1);
		}
		pthread_mutex_lock(&mut);
		int invalid = isReservationValid(order1, o_size);
		writeRequestSlog(thread, invalid, order1, o_size);
		pthread_mutex_unlock(&mut);
		write(fifo, invalid, 1);
		close(fifo);
		if(flag == 1)
			return NULL;
	}
	return NULL;
}

void writeRequestSlog(int thread, int answer, int* request, int size)
{
	char* output = malloc(512);
	snprintf(output, 3, "%d", thread);
	if (strlen(output) == 1)
		write(fdslog, "0", 1);
	write(fdslog, output, 2);
	write(fdslog, "-", 1);
	free(output);
	output = malloc(512);
	snprintf(output, WIDTH_PID+1, "%d", *(request));
	for(int n = strlen(output); n < WIDTH_PID; n++)
		write(fdslog, "0", 1);
	write(fdslog, output, WIDTH_PID);
	write(fdslog, "-", 1);
	free(output);
	output = malloc(512);
	snprintf(output, 3, "%d", *(request+1));
	if(strlen(output) == 1)
		write(fdslog, "0", 1);
	write(fdslog, output, 2);
	write(fdslog, ": ", 2);
	int bought_seats[MAX_CLI_SEATS];
	int index = 0;
	for(int n = 2; n < size; n++)
	{
		free(output);
		output = malloc(512);
		int seat_num = *(request+n);
		snprintf(output, WIDTH_SEAT, "%d", seat_num);
		if (getClientSeat(seats, seat_num) == *request) {
			bought_seats[index] = seat_num;
			index++;
		}
		for(int i = strlen(output); i < WIDTH_SEAT; i++)
			write(fdslog, "0", 1);
		write(fdslog, output, strlen(output));
		write(fdslog, " ", 1);
	}
	write(fdslog, "- ", 2);

	if(answer < 0)
	{
		if (answer == -1)
			write(fdslog, "MAX\n", 4);
		else if (answer == -2)
			write(fdslog, "NST\n", 4);
		else if (answer == -3)
			write(fdslog, "IID\n", 4);
		else if (answer == -4)
			write(fdslog, "ERR\n", 4);
		else if (answer == -5)
			write(fdslog, "NAV\n", 4);
		else if (answer == -6)
			write(fdslog, "FUL\n", 4);
		return;
	}

	for(int i = 0; i < index; i++)
	{
		free(output);
		output = malloc(512);
		snprintf(output, WIDTH_SEAT, "%d", bought_seats[i]);
		for(int j = strlen(output); j < WIDTH_SEAT; j++)
			write(fdslog, "0", 1);
		write(fdslog, output, strlen(output));
		write(fdslog, " ", 1);
	}
	write(fdslog, "\n", 1);
}

int isSeatFree(struct Seat *seats, int seatNum)
{
	int isFree = 0;
	if((seats+(seatNum-1)*sizeof(struct Seat))->clientId <= 0)
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

int getClientSeat(struct Seat *seats, int seatNum)
{
	return (seats+(seatNum-1)*sizeof(struct Seat))->clientId;
}

int isRoomFull(struct Seat *seats)
{
	for(int i = 1; i <= numRoomSeats; i++)
	{
		if(isSeatFree(seats, i))
			return 0;
	}
	return 1;
}

int isReservationValid(int* reservation, int size)
{
	int client_pid = *reservation;
	int num_wanted_seats = *(reservation+1);//sizeof(int));
	if(num_wanted_seats > MAX_CLI_SEATS)
		return -1;
	else if(client_pid < 0 || num_wanted_seats < 0)
		return -4;
	else if(size-2 > MAX_CLI_SEATS || size - 2 < num_wanted_seats)
		return -2;
	else if(isRoomFull(seats))
		return -6;
	int booked_seats = 0;
	for(int i = 2; i < size; i++)
	{
		int seat_num = *(reservation+i);
		if(seat_num < 0)
			return -3;
		if(isSeatFree(seats, seat_num))
		{
			bookSeat(seats, seat_num, client_pid);
			booked_seats++;
		}
		if(booked_seats>=num_wanted_seats)
			break;
	}
	if(booked_seats < num_wanted_seats)
	{
		for (int i = 2; i < size; i++) {
			int seat_num = *(reservation + i);
			if (getClientSeat(seats, seat_num) == client_pid) {
				freeSeat(seats, seat_num);
			}
		}
		return -5;
	}
	return 0;
}

