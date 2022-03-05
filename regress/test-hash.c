/*
 * Copyright (c) 2022 Emil Engler <engler+yhttp@unveil2.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <err.h>
#include <stdlib.h>

#include "../hash.c"

struct name_hash {
	const char	*name;
	size_t		 hash;
};

static void	test_hash(void);
static void	test_hash_init(void);
static void	test_hash_set(void);
static void	test_hash_set1(void);
static void	test_hash_set2(void);
static void	test_hash_set3(void);
static void	test_hash_unset(void);

static const struct name_hash data[] = {
	{ "john", 11 },
	{ "paul", 72 },
	{ "george", 95 },
	{ "ringo", 63 },

	/* Collisions for "john". */
	{ "oxgi", 11 },
	{ "txho", 11 },
	{ "sbpl", 11 },
	{ "tdyt", 11 },

	{ NULL, 0 }
};

/*
 * The internal hashing function does not really require some sort of testing,
 * however we depend on strings to mapped to certain integers inside this
 * regression test.
 */
static void
test_hash(void)
{
	const struct name_hash	*nh;
	size_t			 h;

	for (nh = data; nh->name != NULL; ++nh) {
		h = hash(nh->name);
		if (h != nh->hash)
			errx(1, "hash: have string %s and hash %zu, want hash %zu", nh->name, h, nh->hash);
	}
}

static void
test_hash_init(void)
{
	struct hash	**ht;
	size_t		  i;

	if ((ht = hash_init()) == NULL)
		err(1, "hash_init");

	/* Check if all fields have been initialized to NULL. */
	for (i = 0; i < NHASH; ++i) {
		if (ht[i] != NULL)
			errx(1, "hash_init: ht[%zu] is not NULL", i);
	}

	hash_free(ht);
}

static void
test_hash_set(void)
{
	test_hash_set1();
	test_hash_set2();
	test_hash_set3();
}

static void
test_hash_set1(void)
{
	/* Testing very simple sets across the hash table. */
	struct hash	**ht;
	size_t		  i, h;

	if ((ht = hash_init()) == NULL)
		err(1, "hash_init");

	/* Populate the hash table until ringo. */
	for (i = 0; i < 4; ++i) {
		h = data[i].hash;

		if (hash_set(ht, data[i].name, data[i].name) != YHTTP_OK)
			err(1, "hash_set");

		if (ht[h] == NULL)
			errx(1, "hash_set: ht[%zu] is NULL", h);
		if (strcmp(ht[h]->name, data[i].name) != 0)
			errx(1, "hash_set: name was not set properly");
		if (strcmp(ht[h]->value, data[i].name) != 0)
			errx(1, "hash_set: value was not set properly");
	}

	hash_free(ht);
}

static void
test_hash_set2(void)
{
	/* Testing hash collisions for john. */
	struct hash	**ht;
	struct hash	 *prev;
	size_t		  i, h;

	if ((ht = hash_init()) == NULL)
		err(1, "hash_init");
	if (hash_set(ht, "john", "john") != YHTTP_OK)
		err(1, "hash_set");

	for (i = 4; i < 8; ++i) {
		h = data[i].hash;
		prev = ht[h];

		if (hash_set(ht, data[i].name, data[i].name) != YHTTP_OK)
			err(1, "hash_set");

		if (ht[h]->prev != NULL)
			errx(1, "hash_set: ht[i]->prev is not NULL");
		if (ht[h]->next != prev)
			errx(1, "hash_set: ht[i]->next was not set properly");
		if (prev->prev != ht[h])
			errx(1, "hash_set: prev->next->prev was not set properly");
	}

	hash_free(ht);
}

static void
test_hash_set3(void)
{
	/* Testing the modification of values. */
	struct hash		**ht;
	struct hash		 *node;
	const struct name_hash	 *nh;

	if ((ht = hash_init()) == NULL)
		err(1, "hash_init");

	/* Populate the hash table with the default data. */
	for (nh = data; nh->name != NULL; ++nh) {
		if (hash_set(ht, nh->name, nh->name) != YHTTP_OK)
			err(1, "hash_set");
	}

	/* Set all values to foo. */
	for (nh = data; nh->name != NULL; ++nh) {
		node = hash_get(ht, nh->name);

		if (strcmp(node->value, nh->name) != 0)
			errx(1, "hash_set: value was not set properly");

		if (hash_set(ht, nh->name, "foo") != YHTTP_OK)
			err(1, "hash_set");

		if (strcmp(node->value, "foo") != 0)
			errx(1, "hash_set: value is not foo");
	}

	hash_free(ht);
}

static void
test_hash_unset(void)
{
	struct hash		**ht;
	struct hash		 *prev, *next;
	const struct name_hash	 *nh;

	if ((ht = hash_init()) == NULL)
		err(1, "hash_init");

	/* Populate the hash table with the default data. */
	for (nh = data; nh->name != NULL; ++nh) {
		if (hash_set(ht, nh->name, nh->name) != YHTTP_OK)
			err(1, "hash_set");
	}

	/* Remove ringo. */
	hash_unset(ht, "ringo");
	if (ht[63] != NULL)
		errx(1, "hash_unset: ht[63] is not NULL");

	/* Remove in the middle of the john collision. */
	prev = hash_get(ht, "tdyt");
	next = hash_get(ht, "txho");
	hash_unset(ht, "sbpl");
	if (prev->next != next || next->prev != prev)
		errx(1, "hash_unset: linked list removal failed");

	/* Remove in the end of the john collision. */
	hash_unset(ht, "john");
	if (next->next->next != NULL)
		errx(1, "hash_unset: linked list removal failed");

	hash_free(ht);
}

int
main(int argc, char *argv[])
{
	test_hash();
	test_hash_init();
	test_hash_set();
	test_hash_unset();
	return (0);
}
