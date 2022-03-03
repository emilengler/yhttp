.PHONY: all clean

CFLAGS	+= -std=c99 -g -W -Wall -Wextra -Wpedantic -Wmissing-prototypes
CFLAGS	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter

OBJS	 = yhttp.o

all: libyhttp.a

clean:
	rm -f libyhttp.a ${OBJS}

libyhttp.a: ${OBJS}
	${AR} rcs $@ ${OBJS}
