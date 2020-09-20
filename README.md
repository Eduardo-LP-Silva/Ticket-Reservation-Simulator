# Ticket Reservation Simulator

Application designed to simulate a ticket reservation system in which the clients
are represented by different processes (in the same machine) and each ticket booth by a thread of the main server program.  

## Overview

Each client (independent processes) submits a request to the server (another process), containing:

* The number of seats.
* A list with the prefered seats (the number of elements on this list can be equal or greater than the number of seats).

After each submission, one of the ticket booths (server threads) will process the request, reserving the seat(s) if possible and giving
an anwser to the client.  
Each operation done by the server and clients will be registered in 2 separate text files, slog.txt and clog.txt, respectively. They
will also register in the files sbook.txt and cbook.txt (this last one being shared by all the clients) the number of the seats that
were booked.

## Specifications

### Server

**Command line arguments:** the number of ticket booths, the number of seconds each booth is open for business:

* $ server <num_room_seats> <num_ticket_offices> <open_time>

The communication from the clients to the server is done through a common FIFO (named pipe) named requests. The main thread of this
process is constantly verifying if there are any requests to handle while the ticket booths are open. If that's the case, one of the
booths (other threads) will take on the request and process it.  
If the booking was succesfull, the booth will send the client a list in which the first element indicates the number of seats
effectively booked and the remaining elements the numbers of these seats.  
On the other hand, if it wasn't possible to book the seats, the booth will send the client one of the following negative numbers, each
with its own meaning:

* -1 - The number of wanted seats is greater than the maximum allowed.
* -2 - The number of the prefered seat identifiers is not valid.
* -3 - The identifiers of the prefered seats are invalid.
* -4 - Some other error in the parameters.
* -5 - At least one of the prefered seats is not available (already booked).
* -6 - The room is full.

Each booth will then write to the slog.txt file. Each line will be in one of the following formats:

* TO-OPEN - Signals the opening of the TOth booth.
* TO-CLOSE - Signals the closing of the TOth booth.
* TO-CLIID-NT: aaaa bbbb cccc dddd ... - AAAA BBBB CCCC DDDD ... - Holds information about a succesfull request, being TO the number
of the booth, CLIID the id of the client, NT the number of seats to book, aaaa bbbb ... the identifiers of the prefered seats and
AAAA BBBB ... the identifiers of the actually booked seats.
* TO-CLIID-NT: aaaa bbbb cccc dddd ... - XXX - Holds information about an unsuccessful request. Each respective field has the same meaning mentioned
above, being XXX a code indicating the motive why the request couldn't be fulfilled. This code is tied to the negative number
returned to the client in these situations:
  * -1 - MAX
  * -2 - NST
  * -3 - IID
  * -4 - ERR
  * -5 - NAV
  * -6 - FUL
  
## Client

**Command line arguments:** how much time the client can wait for an anwser, the number of seats the client wants to book, the
identifiers of the prefered seats:

* $ client <time_out> <num_wanted_seats> <pref_seat_list>

Sends the request to the server throught the requests FIFO and waits for an anwser through the FIFO ansXXXXX, being XXXXX the client's
PID. If the elapsed time rises above the time out limit, the client will shut down.  
Either way, it will write to the clog.txt file. Each line will be composed by 2 or 3 fields, depending on the anwser
received:

* The first field represents the client's PID.
* The second field will be in the format XX.NN if the booking was succesfull, being NN the number of booked seats and XX a number
between 1 and NN representing one of the booked seats. If the booking was unsuccessful, the field will have a XXX format, being XXX
one of the codes mentioned above indicating why the booking was unsuccessfull. If the time out limit was reached, it will be written OUT.
* The third field will only appear in the succesfull bookings, indicating the identifier of the seat for each XX.NN item.
