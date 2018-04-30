#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "server.h"


int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return -1;
    }

    int num_room_seats = atoi(argv[1]), num_ticket_offices = atoi(argv[2]), 
        open_time = atoi(argv[3]), *requests;

    mkfifo("requests", 770);

    


    unlink("requests");

}