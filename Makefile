.PHONY: all clean regress

CFLAGS	+= -std=c99 -g -W -Wall -Wextra -Wpedantic -Wmissing-prototypes
CFLAGS	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter
LDFLAGS	+= -L. -lyhttp

OBJS	 = yhttp.o	\
	   hash.o	\
	   buf.o	\
	   parser.o	\
	   net.o	\
	   abnf.o	\
	   util.o	\
	   resp.o
REGRESS	 = regress/test-yhttp_init-free		\
	   regress/test-yhttp_requ-init-free	\
	   regress/test-yhttp_url_enc		\
	   regress/test-yhttp_url_dec		\
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
	   regress/test-parser_rline		\
	   regress/test-parser_header_field	\
	   regress/test-parser_headers		\
	   regress/test-yhttp_resp-init-free	\
	   regress/test-yhttp_resp		\
	   regress/test-util_aprintf

all: libyhttp.a yhttpd

clean:
	rm -f libyhttp.a yhttpd ${OBJS} ${REGRESS}

regress: all ${REGRESS}
	@for f in ${REGRESS}; do	\
		echo ./$${f};		\
		./$${f};		\
	done

libyhttp.a: ${OBJS}
	${AR} rcs $@ ${OBJS}

yhttpd: libyhttp.a yhttpd.c
	${CC} ${CFLAGS} -o $@ yhttpd.c -L. -lyhttp
