/*
 * bcmalloccache.c
 *
 * Implements a working set model for objects alloced. The objects should be of same size.
 * The size of the working set is controlled by using trim level and
 * periodically calling reclaim function.
 * If an object is not found in working set, it's allocated using MALLOC and reclaim will
 * free it using MFREE
 * The locking and timer is application's responsibility.
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: bcmallocache.c,v 1.1 2009/01/05 07:19:05 cchao Exp $
 *
 */
#include <cyg/hal/typedefs.h>
#include <cyg/hal/bcmutils.h>
#ifdef BCMDRIVER
#include <cyg/hal/osl.h>
#include <cyg/hal/siutils.h>
#else
#include <stdio.h>
#include <string.h>
#endif /* BCMDRIVER */
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/bcmallocache.h>
#define CACHE_NAMELEN 20 /* Max length of name for working set instance */

/* Convenient structure to access the buffer
 * When mallocing, data is allocated as
 *          |next_ptr|.....data.....|
 * Buffer points to the begining of data
 */
struct buffer {
	struct buffer *next;
};

struct cache_s {
	osl_t *osh;				/* OS Handle for MALLOC and MFREE */
	struct buffer* head;			/* First element of working set */
	unsigned int 	obj_size;		/* Size of the object */
	char		name[CACHE_NAMELEN];	/* Name for the working set */
#ifdef BCMDBG
	uint hits;	/* Hits to the working set (no MALLOC needed) */
	uint misses;	/* Misses to the working set (no MALLOC needed) */
#endif /* BCMDBG */
};

#define RECLAIM_LEVEL 8	/* Reclaim 1/8th of the cache every time reclaim is called */
#define RECLAIM_TARGET_FACTOR (RECLAIM_LEVEL -1)/RECLAIM_LEVEL

int
bcmcache_count_elems(bcmcache_t *cachep)
{
	int count = 0;
	struct buffer *buf;
	for (buf = cachep->head; buf; buf = buf->next, count++);
	return count;
}

/* Create a working set with given name for given size of element
 * Does not pre-allocate or pre-populate
 */
bcmcache_t *
bcmcache_create(osl_t *osh, char *name, uint size)
{
	bcmcache_t *cachep;

	if ((!name) || ((strlen(name) >= CACHE_NAMELEN - 1))) {
		return NULL;
	}

	if ((cachep = (bcmcache_t *)MALLOC(osh, sizeof(bcmcache_t))) == NULL)
		return NULL;

	bzero(cachep, sizeof(bcmcache_t));
	cachep->obj_size = size;
	cachep->osh = osh;
	strcpy(cachep->name, name);
	return cachep;
}

/* Destroy all the objs in a cache, and release the mem back to the system.
 */
void
bcmcache_destroy(bcmcache_t *cachep)
{
	struct buffer *nextp;
	struct buffer *head;

	head = cachep->head;
	while (head) {
		nextp = head;
		head = head->next;
		MFREE(cachep->osh, nextp, sizeof(struct buffer) + cachep->obj_size);
	}

	MFREE(cachep->osh, cachep, sizeof(bcmcache_t));
}

void *
bcmcache_alloc(bcmcache_t *cachep)
{
	struct buffer *buf;
	buf = cachep->head;

	if (buf) {
		/* Found one, just return immediately */
#ifdef BCMDBG
		cachep->hits++;
#endif /* BCMDBG */
		cachep->head = cachep->head->next;
		return (((uchar *)buf) + sizeof(struct buffer));
	}

	/* Did not find one! allocate */
#ifdef BCMDBG
	cachep->misses++;
#endif /* BCMDBG */
	/* Allocate size + structure link */
	buf = (struct buffer *) MALLOC(cachep->osh,
	                               cachep->obj_size + sizeof(struct buffer));
	/* Data area will actually start after space for next pointer */
	return (buf ? (((uchar *)buf) + sizeof(struct buffer)) : NULL);
}

void
bcmcache_free(bcmcache_t *cachep, void *data)
{
	struct buffer *buf;

	buf = (struct buffer *)(((uchar *)data) - sizeof(struct buffer));
	buf->next = cachep->head;
	cachep->head = buf;
}

/* Reclaim 1/8th of the working set every time invoked.
 * The actual duration is controlled by
 * the application
 */
void
bcmcache_reclaim(bcmcache_t *cachep)
{
	uint count = bcmcache_count_elems(cachep);
	uint reclaim_target = count * RECLAIM_TARGET_FACTOR;
	struct buffer *nextp;
	struct buffer *head;

	head = cachep->head;
	for (; reclaim_target <= count; reclaim_target++) {
		if (head) {
			nextp = head;
			head = head->next;
			MFREE(cachep->osh, nextp, sizeof(struct buffer) + cachep->obj_size);
		}
	}

	cachep->head = head;
}

#ifdef BCMDBG
void
bcmcache_info(bcmcache_t *cachep, char *buf)
{
	buf += sprintf(buf, "name:%s cachep:%p hits:%d misses:%d elems: %d\n", cachep->name,
	               cachep, cachep->hits, cachep->misses, bcmcache_count_elems(cachep));
}
#endif /* BCMDBG */
