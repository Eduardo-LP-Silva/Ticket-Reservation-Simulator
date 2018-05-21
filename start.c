/**
 * OPERATING SYSTEMS (EIC0027 - SOPE) / MIEIC / FEUP
 * TP2, 2017/2018
 * 
 * This file provides the means to create a set of client processes in order to ease both the
 * development and the evaluation of TP2. The program takes its input from a configuration file,
 * whose path is provided as a command line argument. The execution can be halted at any time by 
 * pressing Ctrl+C (sending a SIGINT signal), and it will be either terminated or resumed depending
 * on whether the user confirms the intention to leave (Y/y) or not (any other key), respectively.
 * 
 * The program is structured as follows:
 * 
 * 
 *      (process group G1)                     (process group G2)
 *                                               ,------------, 
 *                                               |  setpgrp() |
 *      __________________                       |            v          redirect stdin
 *     "                  "      fork()       ,~~~~~~~~~~~~~~~~~~, --------------------------,
 *     "  PARENT process  " ----------------> "  LEADER process  "   dup2(fd, STDIN_FILENO)  !
 *     "__________________"                   "~~~~~~~~~~~~~~~~~~" <-------------------------'
 *              '                                    /  |  \
 *              '                                   /   |   \
 *              '  sigaction(SIGINT, ...)          /    |    \ 
 *              '                                 /     |     \
 *              '                                /      |      \
 *              V
 *   _______________________             fork() + execlp("client", ...)
 *  |                       |
 *  |    SIGINT HANDLER     |                /          |          \ 
 *  |-----------------------|               /           |           \
 *  | (send SIGSTOP to G2)  |              /            |            \
 *  |    End processes?     |             /    ...      |     ...     \
 *  |          /\           |            /              |              \
 *  |         /  \          |           /               |               \
 *  |        /    \         |         .  .             .  .            .  .
 *  |       /      \        |      .        .       .        .      .        .
 *  |      /        \       |     .  client  .     .  client  .    .  client  .
 *  |     /          \      |     .    C1    .     .   C...   .    .    Cn    .
 *  | (Y or y)  (any other) |      .        .       .        .      .        .
 *  |    '           '      |         .  .             .  .            .  .
 *  |    v           v      |
 *  |  SIGINT     SIGCONT   |
 *  |  (to G2)    (to G2)   |
 *  |     +                 |
 *  |  exit(0)              |
 *  |_______________________|
 * 
 *
 * The PARENT process is the main process (created by the terminal to run this program), and is the
 * group leader of its process group (i.e., its PID is the same as the PGID). The terminal sends any
 * raised signals (such as SIGINT from pressing Ctrl+C) not to a single process but to a process
 * group (the one of the PARENT process [G1]). Therefore, all other processes are put in a different
 * process group [G2] to avoid receiving those signals. The PARENT process has a handler for SIGINT
 * signal that enables to halt execution (Ctrl+C), and then stop execution (Y/y) or resume it (any
 * other key) by sending, respectively, signals SIGSTOP, SIGINT and SIGCONT to process group G2.
 * 
 * The LEADER process, a child process of the PARENT process, is responsible for setting a new
 * process group [setpgrp()] to create two separate signal handling domains, and for creating a new
 * process for each client (to execute the "client" program [fork() + execlp("client", ...)]). In
 * order to ease input reading (through C library functions), stdin is redirected to the file given.
 * 
 * All other processes are clients that receive their configuration through command line arguments.
 * Their exit status is collected through handle_zombies() function, and the exit status of both the
 * LEADER and the PARENT processes reflect their success: it is only zero (success) if the exit
 * status of all other processes was also zero.
 * 
 * 
 * *** INPUT FORMAT ***
 * 
 * The configuration file must have the following format (any additional whitespaces are ignored):
 * 
 * <start delay in microseconds (us)> <timeout in milliseconds> <number of seats wanted>
 * <list of seat preferences>
 * 
 * where the list of seat preferences is composed of a set of seat numbers followed by END.
 */

// standard C libraries
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// UNIX/Linux libraries
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

// uncomment this line to disable passing invalid arguments to client processes
// (or define it through command line/Makefile)
//#define ADDITIONAL_CHECK

#define MAX_ROOM_SEATS 9999             /* maximum number of room seats/tickets available       */
#define MAX_CLI_SEATS 99                /* maximum number of seats/tickets per request          */
#define WIDTH_PID 5                     /* length of the PID string                             */
#define WIDTH_XXNN 5                    /* length of the XX.NN string (reservation X out of N)  */
#define WIDTH_SEAT 4                    /* length of the seat number id string                  */

// maximum length of the preference list string (the +1 is for the space character)
#define MAX_PREFERENCES_LEN ((WIDTH_SEAT+1)*(MAX_CLI_SEATS))

#define MAX_TOKEN_LEN 1024              /* length of the largest string within config file      */
#define PREF_LIST_END "END"             /* terminator string for the list of seat preferences   */


// macro to quote ("stringify") a value
// [used by create_client_process() to create string format according to WIDTH_SEAT]
#define QUOTE(str) __QUOTE(str)
#define __QUOTE(str)  #str

// ANSI colors for terminal coloring
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
 * @brief Enumeration to increase the readability of exit status.
 * 
 */
enum TP2_Exit_Status {
  INVALID_CMD_ARGS = 1,                 // the set of arguments is invalid
  FILE_OPEN_FAILED,                     // unable to open configuration file
  SIG_HANDLER_ERR,                      // unable to install signal handler
  FORK_FAILED,                          // unable to create a new process
  SET_PGID_FAILED,                      // unable to set process group
  STDIN_REDIR_ERR,                      // unable to redirect standard input
  CLIENT_RUNTIME_ERR,                   // one or more client processes failed (exit status != 0)
  CLIENT_EXEC_FAILED                    // unable to execute client program (execlp call failed)
};

/**
 * @brief Enumeration to increase the readability of read_client_info() function's return status.
 * 
 */
enum ReadClientInfoStatus {
  INPUT_ENDED = 0,                      // EOF reached
  INVALID_POINTER = -11,                // pointer to struct client_info object is NULL
  INVALID_START_DELAY = -12,            // unable to read/parse the start delay
  INVALID_NUM_WANTED_SEATS = -13,       // unable to read/parse the number of seats/tickets wanted
  INVALID_SEAT_NUMBER = -14,            // unable to read/parse a seat number (list of preferences)
  LIST_PREF_TOO_SHORT = -15,            // the list of preferences is too short (< #seats/tickets)
  INVALID_TIMEOUT = -16                 // unable to read/parse client timeout
};

/**
 * @brief Structure to hold the required information about a given client.
 * 
 *            seq_no: relative order number within the file (useful for error messages);
 *          delay_us: the time offset, in microseconds and relative to the previous client, when the
 *                    client should run [before invoking fork()];
 *           timeout: client timeout in milliseconds;
 *  num_wanted_seats: number of seats/tickets requested by the client;
 *       preferences: the list of preferences (seat numbers).
 */
struct client_info {
  int seq_no;                           // sequence number (client relative order)
  int delay_us;                         // delay in microseconds
  int timeout_ms;                       // client timeout in milliseconds
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS*2];     // list of preferences (seat numbers)
  int npreferences;                     // length of the list of preferences
};

// global variable holding the PGID of all client processes and of the LEADER process [G2]
// (needed inside the interrupt handler to stop, continue or terminate client processes)
pid_t cgroup;

//
// Ancillary functions (see description along their definition)
//

static bool redirect_stdin(int fd);

static int read_client_info(struct client_info *ci);

static pid_t create_client_process(const struct client_info *ci);

static int handle_zombies(int flags);

static int main_loop();

/**
 * @brief Prints an error message to standard error.
 *
 * if errno is set, sterror() is used to provide a textual representation of it.
 * 
 * @param fmt Format (as of printf).
 */
static void log_error(const char *fmt, ...) {
  va_list args;

  // initialize variadic argument list
  va_start(args, fmt);

  // print error message preamble ("\n\n** [<process PID>] ")
  fprintf(
    stderr,
    "\n%s************************%s [PID %0" QUOTE(WIDTH_PID) "d] %s************************%s\n",
    ANSI_COLOR_RED, ANSI_COLOR_WHITE, getpid(), ANSI_COLOR_RED, ANSI_COLOR_WHITE
  );

  // print the format string given using the provided arguments, if any
  // [vprintf() is used to enable passing the list of variadic arguments]
  vfprintf(stderr, fmt, args);

  // if errno is set, include the corresponding error string
  if(errno != 0)
    fprintf(stderr, ": %s", strerror(errno));
  
  // print the epilogue
  fprintf(
    stderr,
    "!\n%s*************************************************************%s\n",
    ANSI_COLOR_RED, ANSI_COLOR_RESET
  );

  // end variadic argument list
  va_end(args);
}

/**
 * @brief SIGINT handler.
 * 
 * Handles SIGINT from the terminal (sent to all processes within process group G1) and stops the
 * execution of all child processes (put in process group G2).
 * 
 * @param signo The signal being received.
 */
static void sigint_handler(int signo) {
  char answer;

  // send SIGSTOP signal to all client processes and to the LEADER process [process group G2]
  kill(-cgroup, SIGSTOP);

  printf(
    "\n\nDo you want to stop all client processes? (Y to confirm, any other key to continue)\n"
  );
  // read a character from stdin and ignore all other characters ("clears" stdin)
  scanf("%c%*[^\n]%*c", &answer);

  // if the user intends to stop
  if (answer == 'y' || answer == 'Y') {
    // send a SIGINT signal to all other processes (process group G2)
    kill(-cgroup, SIGINT);

    // and terminate the program
    exit(0);
  }

  // otherwise, print a message
  printf("Resuming...\n");

  // and send SIGCONT signal to process group G2 in order to resume LEADER and client processes
  kill(-cgroup, SIGCONT);
}


/**
 * @brief Main function.
 */
int main(int argc, char* argv[]) {
  struct sigaction sa;
  int fd, ret = 0;

  // takes a single argument (input file)
  if (argc != 2) {
    printf("Usage: %s <input file>\n", argv[0]);

    return INVALID_CMD_ARGS;
  }

  // open the file
  if ((fd = open(argv[1], O_RDONLY)) == -1) {
    // on failure, print the reason why
    log_error("Unable to open file");

    return FILE_OPEN_FAILED;
  }

  // set SIGINT handler (struct sigaction object)
  sa.sa_handler = sigint_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  // install it
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    // on failure, print an error message
    log_error("Unable to install handler");

    // and close the file
    close(fd);
    
    return SIG_HANDLER_ERR;
  }

  // create the LEADER process
  switch((cgroup = fork())) {
    /* failure */
    case -1:
      log_error("Unable to create LEADER process");
      
      ret = FORK_FAILED;

      break;

    /* child */
    case 0:
      // default SIGINT handler (terminate process)
      sa.sa_handler = SIG_DFL;

      // install the default SIGINT handler (only the PARENT process will handle SIGINT)
      if (sigaction(SIGINT, &sa, NULL) == -1) {
        // on failure, print an error message
        log_error("Unable to install handler");

        // and close the file
        close(fd);
        
        return SIG_HANDLER_ERR;
      }

      // create a new process group [G2] so that signals sent by the terminal are no longer received
      if (setpgrp() == -1) {
        // on failure, print an error message
        log_error("Unable to set group id");

        return SET_PGID_FAILED;
      }

      // redirect stdin to ease the use of C libraries (by default, input is read from stdin)
      // [redirect_stdin() already closes the file descriptor within as it is no longer needed]
      if(!redirect_stdin(fd))
        return STDIN_REDIR_ERR;

      // main loop to read each client's information and to create client processes
      return (main_loop() != 0) ? CLIENT_RUNTIME_ERR : 0;

    /* parent */
    default:
      // close file (this process will not read the input file)
      close(fd);

      // wait for the LEADER process
      ret = (handle_zombies(0) != 0) ? CLIENT_RUNTIME_ERR : 0;
  }

  return ret;
}

//
// Ancillary functions
//

/**
 * @brief Redirects standard input and closes the file descriptor (if valid).
 * 
 * This function redirects the standard input to a given file descriptor. The function checks
 * whether the file descriptor is valid and if the redirection succeeded. The file descriptor is
 * closed in either way, unless it is not valid: on success, it is no longer needed; on failure, the
 * program will terminate.
 * 
 * @param fd The file descriptor that the standard input (STDIN_FILENO) should be redirected to.
 * 
 * @return A boolean indicating whether redirection succeeded or failed.
 */
static bool redirect_stdin(int fd) {
  // check if this is a valid descriptor
  if (fcntl(fd, F_GETFD) == -1) {
    // if it is not, print an error message
    log_error("Invalid file descriptor");

    return false;
  }

  // redirect stdin to the file
  if(dup2(fd, STDIN_FILENO) == -1) {
    // if it fails, print the reason why
    log_error("Unable to redirect standard input");

    // close the file because the program will terminate
    close(fd);

    return false;
  }

  // if everything succeeds, only one file descriptor is needed (STDIN_FILENO)
  close(fd);

  return true;
}

/**
 * @brief Reads the information respecting a given client.
 * 
 * read_client_info attempts to read the required client information in the following format:
 *    <start delay in us> <timeout in ms> <no. of seats/tickets wanted> <list of seat preferences>.
 * 
 * Note: the list of seat preferences ends when a terminator string is found (macro PREF_LIST_END).
 * 
 * If an error occurs, the client number (relative order within the file) is printed along an error
 * message; if the error regards a seat number, its index is also printed.
 * 
 * @param ci A pointer to the struct client_info object that is to be set.
 * 
 * @return int On success, the size of the list; on failure, an error code (negative value).
 */
static int read_client_info(struct client_info *ci) {
  static int seq_no = 0;

  const size_t plen = strlen(PREF_LIST_END);
  char buf[MAX_TOKEN_LEN];
  int idx = -1, ret;

  // check if the pointer is not null
  if (ci == NULL) {
    log_error("Invalid pointer for struct client_info");

    return INVALID_POINTER;
  }

  // set sequence number
  ci->seq_no = ++seq_no;

  // clear list of preferences
  memset(ci->preferences, 0x00, sizeof(ci->preferences));

  // start with an empty list of preferences
  ci->npreferences = 0;

  // if the start delay could not be read or if it is invalid
  if(((ret = scanf("%d", &ci->delay_us)) != 1) || (ci->delay_us < 0)) {
    // if there is nothing else to read, return zero to let the caller know
    if (ret == EOF)
      return INPUT_ENDED;

    // otherwise, print an error message and report it
    if (ret != 1)
      log_error("Unable to read the start delay (client #%d)", seq_no);
    else
      log_error("Invalid start delay (client #%d): %d", seq_no, ci->delay_us);

    return INVALID_START_DELAY;
  }

  // if an error occurred while reading the timeout
  if(((ret = scanf("%d", &ci->timeout_ms)) != 1) || (ci->timeout_ms < 1)) {
    // report it
    if (ret != 1)
      log_error("Unable to read client timeout (client #%d)", seq_no);
    else
      log_error("Invalid client timeout (client #%d): %d", seq_no, ci->timeout_ms);

    return INVALID_TIMEOUT;
  }

  // if an error occurred while reading the number of seats/tickets wanted
  if(scanf("%d", &ci->num_wanted_seats) != 1) {
    // report it
    log_error("Unable to read the no. of seats/tickets wanted (client #%d)", seq_no);

    return INVALID_NUM_WANTED_SEATS;
  }

#ifdef ADDITIONAL_CHECK
  // check if the number of wanted seats is valid
  if((ci->num_wanted_seats < 1) || (ci->num_wanted_seats >= MAX_CLI_SEATS)) {
    log_error("Invalid no. of seats wanted (client #%d): %d", seq_no, ci->num_wanted_seats);

    return INVALID_NUM_WANTED_SEATS;
  }
#endif

  // read the list of preferences (list of seat numbers)
  do {
    // if an error occurred while reading the seat number
    if(scanf("%d", &ci->preferences[++idx]) != 1) {
      // read the input as a string (of size no larger than MAX_TOKEN_LEN characters)
      scanf("%"QUOTE(MAX_TOKEN_LEN)"s", buf);

      // break the loop if it is the terminator for the list of seat preferences
      if (!strncmp(PREF_LIST_END, buf, plen) && (strlen(buf) == plen))
        break;

      // otherwise, print client and seat number indices, and return the error
      log_error("Unable to read the seat number (client #%d, list element #%d)!\n", seq_no, idx+1);

      return INVALID_SEAT_NUMBER;
    }



#ifdef ADDITIONAL_CHECK
    // check if the seat number is valid
    if ((ci->preferences[idx] < 1) || (ci->preferences[idx] > MAX_ROOM_SEATS)) {
      // if the seat number is not valid, print also its value
      log_error(
        "Invalid seat number [%d] (client #%d, list element #%d)",
        ci->preferences[idx], seq_no, idx+1
      );

      return INVALID_SEAT_NUMBER;
    }
#endif
  }
  while (idx < sizeof(ci->preferences)/sizeof(ci->preferences[0]));

#ifdef ADDITIONAL_CHECK
  // check if the list of seat preferences is large enough for the number of seats/tickets requested
  if (idx < ci->num_wanted_seats) {
    log_error(
      "List of seat preferences is too short (client #%d):\n"
      "  requested %d seat(s)/ticket(s) but list has size %d", seq_no, ci->num_wanted_seats, idx
    );

    return LIST_PREF_TOO_SHORT;
  }
#endif

  ci->npreferences = idx;

  // return the size of the list of preferences
  return idx;
}

/**
 * @brief Create a client process and execute it.
 * 
 * This function creates a new process to run a client program and passes its configuration through
 * command line argument list:
 *   <prog. name> <no. of req. tickets> <list of preferences (comma separated d-digit seat numbers)>
 * 
 * @param ci A pointer to constant object containing the client information.
 * 
 * @return pid_t The PID of the process created or -1 if fork() failed.
 */
static pid_t create_client_process(const struct client_info *ci) {
  char timeout[MAX_TOKEN_LEN];
  char num_wanted_seats[WIDTH_SEAT+1];
  char preferences[MAX_PREFERENCES_LEN];
  pid_t pid;
  int i, idx;
  
  // create a new process
  switch((pid = fork())) {
    /* failure */
    case -1:
      log_error("Unable to create CLIENT process");
      
      break;
    
    /* child */
    case 0:
      // create the argument string that holds the client timeout
      snprintf(timeout, sizeof(timeout), "%d", ci->timeout_ms);

      // create the argument string that holds the number of seats/tickets wanted
      snprintf(num_wanted_seats, sizeof(num_wanted_seats), "%d", ci->num_wanted_seats);

      // create the argument string that holds the list of preferences
      for(i = 0, idx = 0; i < MAX_ROOM_SEATS && i < ci->npreferences; ++i)
        idx += snprintf(&preferences[idx], MAX_PREFERENCES_LEN - idx, "%d ", ci->preferences[i]);

      // replace last space of the list of preferences by the null character
      preferences[idx-1] = '\0';

      // execute client process
      execlp("./client", "./client", timeout, num_wanted_seats, preferences, NULL);

      //
      // code only reaches here if execlp failed
      //

      // close file (stdin has been redirected to the configuration file)
      close(STDIN_FILENO);

      // print error message
      log_error("Unable to run client process");

      // terminate process
      exit(CLIENT_EXEC_FAILED);

    default: break;
  }

  // return the pid of the process created (or -1 if fork call failed)
  return pid;
}

/**
 * @brief Waits for child processes to avoid any zombies.
 * 
 * This function behaves differently depending on whether the WNOHANG flag has been set:
 *   1) if set, reads the exit status of all zombie processes (return value > 0);
 *   2) if not set, waits for any child processes (return value > -1).
 * 
 * In either case, if an interrupt has been raised, errno is cleared and waitpid is invoked again.
 * 
 * @param flags Any flags supported by waitpid.
 * 
 * @return int The combined exit status (a binary or) of all child processes that have ended.
 */
static int handle_zombies(int flags) {
  // if WNOHANG is set, clears all zombies; if not, waits for all running child processes
  int criterion = (flags & WNOHANG) ? 0 : -1;
  pid_t pid;
  int ret = 0, status = 0;

  // while criterion is matched or an interrupt has been raised
  while(((pid = waitpid(-1, &status, flags)) > criterion) || errno == EINTR) {
    // in case of an interrupt, clear errno and try again
    if (errno == EINTR) {
      errno = 0;

      continue;
    }

    // otherwise, print a message and combine its exit status
    printf("%sENDED: %d (exit status = %d)%s\n", ANSI_COLOR_GREEN, pid, status, ANSI_COLOR_RESET);

    ret |= status;
  }
  
  // return the combined exit status
  return ret;
}

/**
 * @brief Main loop for the LEADER process.
 * 
 * The function runs while there are clients to be setup, and, for each one of them, goes to sleep
 * in order to introduce the required delay and creates a new process to execute it.
 * 
 * @return int The combined exit status (a binary or) of all child processes that have ended.
 */
static int main_loop() {
  struct client_info ci;
  int i, ret;
  pid_t pid;

  // read the setup for each client
  while((ret = read_client_info(&ci)) > 0) {
    // introduce the required delay
    usleep(ci.delay_us);

    // run the client process with the proper configuration
    pid = create_client_process(&ci);

    // if client process was not created, return failure
    if (pid == -1)
      return FORK_FAILED;

    // otherwise, print its configuration, PID and PGID
    printf(
      "%s-------------------------------------------------------------%s\n",
      ANSI_COLOR_YELLOW, ANSI_COLOR_RESET
    );

    printf(
      "CLIENT #%d (PID %d, PGID %d)\n\nDelay: %d us\nTimeout: %d ms\n",
      ci.seq_no, pid, getpgid(pid), ci.delay_us, ci.timeout_ms
    );
    printf("#Seats/Tickets: %d\nPreferences: ", ci.num_wanted_seats);

    // list of preferences
    for(i = 0; (i < MAX_ROOM_SEATS) && (i < ci.npreferences); ++i)
      printf("%d ", ci.preferences[i]);

    printf(
      "\n%s-------------------------------------------------------------%s\n",
      ANSI_COLOR_YELLOW, ANSI_COLOR_RESET
    );

    // avoid any zombie processes and combine their exit status
    ret |= handle_zombies(WNOHANG);
  }

  // return the combined exit status
  return (ret | handle_zombies(0));
}
