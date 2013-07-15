EXEC=tilel
LIBS=xcb-ewmh

SRCS=$(wildcard *.c)

CFLAGS=-std=c99 -Wall -Wextra -pedantic -s -pipe
LDFLAGS=`pkg-config --cflags --libs ${LIBS}`

DEBUG=-O0 -g
RELEASE=-O2 -DNDEBUG

PREFIX?= /usr/local
BINDIR?= ${PREFIX}/bin

all: release

debug: CFLAGS+=${DEBUG}
debug: ${EXEC}

release: CFLAGS+=${RELEASE}
release: ${EXEC}

${EXEC}: clean
	${CC} ${CFLAGS} ${LDFLAGS} ${SRCS} -o ${EXEC}

clean:
	rm -f ${EXEC}

run: debug
	valgrind --track-origins=yes --leak-check=full ./${EXEC}

install: release
	install -Dm 755 ${EXEC} ${DESTDIR}${BINDIR}/${EXEC}
