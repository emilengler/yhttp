.PHONY: all clean regress

CFLAGS	+= -std=c99 -g -W -Wall -Wextra -Wpedantic -Wmissing-prototypes
CFLAGS	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter
LDFLAGS	+= -L. -lyhttp

OBJS	 = yhttp.o	\
	   hash.o	\
	   buf.o	\
	   parser.o	\
	   net.o
REGRESS	 = regress/test-init-free		\
	   regress/test-requ-init-free		\
	   regress/test-url_enc			\
	   regress/test-url_dec			\
	   regress/test-hash			\
	   regress/test-buf			\
	   regress/test-parser-init-free	\
	   regress/test-net_poll		\
	   regress/test-parser_find_eol		\
	   regress/test-parser_keyvalue		\
	   regress/test-parser_query		\
	   regress/test-parser_rline_method	\
	   regress/test-parser_rline_path	\
	   regress/test-parser_rline_target	\
	   regress/test-parser_rline

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
