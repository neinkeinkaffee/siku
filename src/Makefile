CC = g++
CFLAGS = -Wall -g

	
util.o: util.cpp util.h
	${CC} ${CFLAGS} -c util.cpp

document.o: document.cpp document.h
	${CC} ${CFLAGS} -c document.cpp

copyfind.o: copyfind.cpp document.h
	${CC} ${CFLAGS} -c copyfind.cpp

copy: copyfind.o document.o
	${CC} ${CFLAGS} copyfind.o util.o document.o -o copy
