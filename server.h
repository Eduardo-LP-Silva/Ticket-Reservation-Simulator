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
int requests, fdslog, numRoomSeats, order_size, flag = 0;
int order[1][MAX_CLI_SEATS];
pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER; //to receive order
pthread_mutex_t mut2=PTHREAD_MUTEX_INITIALIZER; //to make reservation
pthread_mutex_t mut3=PTHREAD_MUTEX_INITIALIZER; //to write on file

struct Seat
{
	int numSeat;
	int clientId; //if client_pid < 0 -> seat is free
};

/**
 * Função executada pelas threads que simbolizam as bilheteiras. Recebe como argumento o número da thread.
 * Verifica se um pedido de um cliente é ou não válido, reservando os lugares caso seja e mandando uma
 * resposta ao cliente através da fifo ansXXXXX, onde XXXXX é o pid do cliente. Esta resposta é um número
 * negativo caso o pedido seja inválido, ou os números dos lugares reservados, caso o pedido tenha sido
 * executado.
 */
void *handleReservations(void *arg);
/**
 * Verifica se o lugar seatNum está livre.
 */
int isSeatFree(struct Seat *seats, int seatNum);
/**
 * Reserva o lugar seatNum ao cliente de pid: clientId.
 */
void bookSeat(struct Seat *seats, int seatNum, int clientId);
/**
 * Cansela a reserva do lugar seatNum.
 */
void freeSeat(struct Seat *seats, int seatNum);
/**
 * Cria seatNum lugares na sala.
 */
struct Seat* createSeats(int numSeats);
/**
 * Escreve no ficheiro slog.txt a abertura ou fecho da thread número 'thread', de acordo com o valor da
 * variável open (1 - abriu a bilheteira; senão, fechou a bilheteira).
 */
void writeOpenCloseLogFile(int thread, int open);
/**
 * Escreve no ficheiro sbook.txt os lugares que foram reservados. É executada no final do programa.
 */
void writeBookingsFile();
/**
 * Verifica se o pedido reservation de um cliente, de tamanho size, é válido, reservando os lugares
 * em caso afirmativo, ou lretornando um valor negativo, em caso negativo.
 * Esta função vai reservando os lugares até que tenham sido reservados um número suficiente de lugares ou
 * até que se conclua que não é possível validar o pedido, libertando os lugares reservados.
 */
int isReservationValid(int* reservation, int size);
/**
 * Escreve no ficheiro slog.txt o pedido do cliente e se ele foi ou não executado.
 * Recebe como argumentos o identificador da bilheteira (thread), a resposta da bilheteira ao pedido
 * (answer), o pedido do cliente (request) e o tamanho do pedido (size).
 */
void writeRequestSlog(int thread, int answer, int* request, int size);
/**
 * Verifica se os lugares foram todos reservados, retornando 1 em caso afirmativo e 0 em caso negativo.
 */
int isRoomFull(struct Seat *seats);
/**
 * Retorna o pid do cliente que reservou o lugar de número seatNum.
 */
int getClientSeat(struct Seat *seats, int seatNum);
