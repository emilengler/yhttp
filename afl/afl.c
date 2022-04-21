#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void	afl(unsigned char *, size_t);

static void
afl(unsigned char *data, size_t ndata)
{
}

int
main(int argc, char *argv[])
{
	unsigned char	*data, buf[128];
	size_t		 i;
	int		 c;

	i = 0;
	while ((c = getchar()) != EOF && i < sizeof(buf))
		buf[i++] = c;

	/*
	 * When we copy buf into an allocated buffer that is exactly of the
	 * required size, we can make use of guard pages detecting potential
	 * out-of-band reads and writes.
	 */
	if ((data = malloc(i)) == NULL)
		return(1);
	memcpy(data, buf, i);

	afl(data, i);

	free(data);

	return (0);
}
