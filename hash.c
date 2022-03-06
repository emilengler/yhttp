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

#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "yhttp.h"

#define NHASH	128

static size_t	hash(const char *);

/*
 * The hashing function was inspired by the one found in chapter 2.9 of the
 * book "The Practice of Programming" from Brian W. Kernighan and Rob Pike.
 * It is too simple in order to be eligible for any sort of copyright.
 */
static size_t
hash(const char *s)
{
	const unsigned char	*p;
	size_t			 h;

	h = 0;
	for (p = (const unsigned char *)s; *p != '\0'; ++p)
		h = (31 * h) + *p;

	return (h % NHASH);
}

struct hash **
hash_init(void)
{
	struct hash	**ht;
	size_t		  i;

	/*
	 * Allocate an array of struct hash pointers, where each of them is
	 * the head node of a linked list for that specific index.
	 */
	if ((ht = malloc(sizeof(struct hash) * NHASH)) == NULL)
		return (NULL);

	/* Initialize all pointers to NULL. */
	for (i = 0; i < NHASH; ++i)
		ht[i] = NULL;

	return (ht);
}

void
hash_free(struct hash *ht[])
{
	struct hash	*tmp;
	size_t		 i;

	/* Free all linked lists inside the hash table array. */
	for (i = 0; i < NHASH; ++i) {
		while (ht[i] != NULL) {
			tmp = ht[i];
			ht[i] = ht[i]->next;

			free(tmp->name);
			free(tmp->value);
			free(tmp);
		}
	}

	free(ht);
}

struct hash *
hash_get(struct hash *ht[], const char *name)
{
	struct hash	*node;
	size_t		 h;

	h = hash(name);
	for (node = ht[h]; node != NULL; node = node->next) {
		if (strcmp(name, node->name) == 0)
			break;
	}

	return (node);
}

int
hash_set(struct hash *ht[], const char *name, const char *value)
{
	struct hash	*node;
	size_t		 h;

	/* Check if the entry already exists. */
	if ((node = hash_get(ht, name)) != NULL) {
		/* The entry already exists, overwrite value. */
		free(node->value);
		if ((node->value = strdup(value)) == NULL)
			return (YHTTP_ERRNO);
	} else {
		/* The entry does not exist yet, create and add it. */
		if ((node = malloc(sizeof(struct hash))) == NULL)
			return (YHTTP_ERRNO);
		node->name = NULL;
		node->value = NULL;

		if ((node->name = strdup(name)) == NULL)
			goto err;
		if ((node->value = strdup(value)) == NULL)
			goto err;

		/* Insert the new node into the front of the linked list. */
		h = hash(name);
		node->prev = NULL;
		node->next = ht[h];
		if (node->next != NULL)
			node->next->prev = node;
		ht[h] = node;
	}

	return (YHTTP_OK);
err:
	free(node->name);
	free(node->value);
	free(node);
	return (YHTTP_ERRNO);
}

void
hash_unset(struct hash *ht[], const char *name)
{
	struct hash	*node;
	size_t		 h;

	/* Do nothing if the entry does not even exist in the first place. */
	if ((node = hash_get(ht, name)) == NULL)
		return;

	h = hash(name);

	/* Remove the node from the linked list. */
	if (node->prev == NULL) {
		/* The node is the head node. */
		if (node->next != NULL)
			node->next->prev = NULL;
		ht[h] = node->next;
	} else {
		/* The node is just a regular or the last node. */
		if (node->next != NULL)
			node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	/* Delete the node. */
	free(node->name);
	free(node->value);
	free(node);
}
