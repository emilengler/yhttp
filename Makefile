.PHONY: all clean regress

CFLAGS	+= -std=c99 -g -W -Wall -Wextra -Wpedantic -Wmissing-prototypes
CFLAGS	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter
LDFLAGS	+= -L. -lyhttp

OBJS	 = yhttp.o
REGRESS	 = regress/test-url_enc

all: libyhttp.a

clean:
	rm -f libyhttp.a ${OBJS} ${REGRESS}

regress: all ${REGRESS}
	@for f in ${REGRESS}; do	\
		echo ./$${f};		\
		./$${f};		\
	done

libyhttp.a: ${OBJS}
	${AR} rcs $@ ${OBJS}
