EXEC=tilel
LIBS=xcb-ewmh

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

CFLAGS=-std=c99 -Wall -Wextra -pedantic -s -pipe
LDFLAGS=`pkg-config --cflags --libs ${LIBS}`

DEBUG=-O0 -g
RELEASE=-Ofast -DNDEBUG

PREFIX?= /usr/local
BINDIR?= ${PREFIX}/bin

all: release

debug: CFLAGS+=${DEBUG}
debug: ${EXEC}

release: CFLAGS+=${RELEASE}
release: clean ${EXEC}

${EXEC}: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} -o ${EXEC}

clean:
	rm -f ${OBJS}

run: debug
	valgrind --track-origins=yes --leak-check=full ./${EXEC}

install: release
	install -Dm 755 ${EXEC} ${DESTDIR}${BINDIR}/${EXEC}
