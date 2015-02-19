CC = gcc

LIBS = -lresolv -lnsl -lpthread\
    /home/courses/cse533/Stevens/unpv13e/libunp.a\
   
FLAGS = -g -O2

CFLAGS =	${FLAGS} -I/home/courses/cse533/Stevens/unpv13e/lib

PROGS = odr_cse533-5 client_cse533-5 server_cse533-5

all:	${PROGS}

client_cse533-5: client.o get_hw_addrs.o
	${CC}	${FLAGS} -o client_cse533-5 client.o get_hw_addrs.o	${LIBS} -lm
client.o: client.c
	${CC}	${CFLAGS} -c client.c	${LIBS}


odr_cse533-5: odr.o get_hw_addrs.o
	${CC}	${FLAGS} -o odr_cse533-5 odr.o get_hw_addrs.o ${LIBS} -lm
odr.o: odr.c
	${CC}	${CFLAGS} -c odr.c	${LIBS}

server_cse533-5: server.o get_hw_addrs.o
	${CC}	${FLAGS} -o server_cse533-5 server.o get_hw_addrs.o ${LIBS} -lm
server.o: server.c
	${CC}	${CFLAGS} -c server.c	${LIBS}



get_hw_addrs.o: get_hw_addrs.c
	${CC}	${CFLAGS} -c get_hw_addrs.c	${LIBS}

clean:
	rm -f	${PROGS}	${CLEANFILES}
