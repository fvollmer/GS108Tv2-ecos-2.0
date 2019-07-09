/*
 * Misc useful OS-independent routines.
 *
 * Copyright 2004, Broadcom Corporation      
 * All Rights Reserved.      
 *       
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
 * $Id: bcmutils.c,v 1.1.2.1 Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/bcmnvram.h>

unsigned char bcm_ctype[] = {
	_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,			/* 0-7 */
	_BCM_C,_BCM_C|_BCM_S,_BCM_C|_BCM_S,_BCM_C|_BCM_S,_BCM_C|_BCM_S,_BCM_C|_BCM_S,_BCM_C,_BCM_C,		/* 8-15 */
	_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,			/* 16-23 */
	_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,_BCM_C,			/* 24-31 */
	_BCM_S|_BCM_SP,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,			/* 32-39 */
	_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,			/* 40-47 */
	_BCM_D,_BCM_D,_BCM_D,_BCM_D,_BCM_D,_BCM_D,_BCM_D,_BCM_D,			/* 48-55 */
	_BCM_D,_BCM_D,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,			/* 56-63 */
	_BCM_P,_BCM_U|_BCM_X,_BCM_U|_BCM_X,_BCM_U|_BCM_X,_BCM_U|_BCM_X,_BCM_U|_BCM_X,_BCM_U|_BCM_X,_BCM_U,	/* 64-71 */
	_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,			/* 72-79 */
	_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,			/* 80-87 */
	_BCM_U,_BCM_U,_BCM_U,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,			/* 88-95 */
	_BCM_P,_BCM_L|_BCM_X,_BCM_L|_BCM_X,_BCM_L|_BCM_X,_BCM_L|_BCM_X,_BCM_L|_BCM_X,_BCM_L|_BCM_X,_BCM_L,	/* 96-103 */
	_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,			/* 104-111 */
	_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,			/* 112-119 */
	_BCM_L,_BCM_L,_BCM_L,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_C,			/* 120-127 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 128-143 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 144-159 */
	_BCM_S|_BCM_SP,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,   /* 160-175 */
	_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,_BCM_P,       /* 176-191 */
	_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,       /* 192-207 */
	_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_P,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_U,_BCM_L,       /* 208-223 */
	_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,       /* 224-239 */
	_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_P,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L,_BCM_L        /* 240-255 */
};

uchar
bcm_toupper(uchar c)
{
	if (bcm_islower(c))
		c -= 'a'-'A';
	return (c);
}

ulong
bcm_strtoul(char *cp, char **endp, uint base)
{
	ulong result, value;
	bool minus;
	
	minus = FALSE;

	while (bcm_isspace(*cp))
		cp++;
	
	if (cp[0] == '+')
		cp++;
	else if (cp[0] == '-') {
		minus = TRUE;
		cp++;
	}
	
	if (base == 0) {
		if (cp[0] == '0') {
			if ((cp[1] == 'x') || (cp[1] == 'X')) {
				base = 16;
				cp = &cp[2];
			} else {
				base = 8;
				cp = &cp[1];
			}
		} else
			base = 10;
	} else if (base == 16 && (cp[0] == '0') && ((cp[1] == 'x') || (cp[1] == 'X'))) {
		cp = &cp[2];
	}
		   
	result = 0;

	while (bcm_isxdigit(*cp) &&
	       (value = bcm_isdigit(*cp) ? *cp-'0' : bcm_toupper(*cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (minus)
		result = (ulong)(result * -1);

	if (endp)
		*endp = (char *)cp;

	return (result);
}

uint
bcm_atoi(char *s)
{
	uint n;

	n = 0;

	while (bcm_isdigit(*s))
		n = (n * 10) + *s++ - '0';
	return (n);
}

void
deadbeef(char *p, uint len)
{
	static uchar meat[] = { 0xde, 0xad, 0xbe, 0xef };

	while (len-- > 0) {
		*p = meat[((uint)p) & 3];
		p++;
	}
}

/* pretty hex print a contiguous buffer */
void
prhex(char *msg, uchar *buf, uint nbytes)
{
	char line[256];
	char* p;
	uint i;

	if (msg && (msg[0] != '\0'))
		printf("%s: ", msg);

	p = line;
	for (i = 0; i < nbytes; i++) {
		if (i % 16 == 0) {
			p += sprintf(p, "%04d: ", i);	/* line prefix */
		}
		p += sprintf(p, "%02x ", buf[i]);
		if (i % 16 == 15) {
			printf("%s\n", line);		/* flush line */
			p = line;
		}
	}

	/* flush last partial line */
	if (p != line)
		printf("%s\n", line);
}

/* pretty hex print a pkt buffer chain */
void
prpkt(char *msg, void *drv, void *p0)
{
	void *p;

	if (msg && (msg[0] != '\0'))
		printf("%s: ", msg);

	for (p = p0; p; p = PKTNEXT(drv, p))
		prhex(NULL, PKTDATA(drv, p), PKTLEN(drv, p));
}

/* copy a pkt buffer chain into a buffer */
uint
pktcopy(void *drv, void *p, uint offset, int len, uchar *buf)
{
	uint n, ret = 0;

	if (len < 0)
		len = 4096;	/* "infinite" */

	/* skip 'offset' bytes */
	for (; p && offset; p = PKTNEXT(drv, p)) {
		if (offset < (uint)PKTLEN(drv, p))
			break;
		offset -= PKTLEN(drv, p);
	}

	if (!p)
		return 0;

	/* copy the data */
	for (; p && len; p = PKTNEXT(drv, p)) {
		n = MIN((uint)PKTLEN(drv, p) - offset, (uint)len);
		bcopy(PKTDATA(drv, p) + offset, buf, n);
		buf += n;
		len -= n;
		ret += n;
		offset = 0;
	}

	return ret;
}

/* return total length of buffer chain */
uint
pkttotlen(void *drv, void *p)
{
	uint total;

	total = 0;
	for (; p; p = PKTNEXT(drv, p))
		total += PKTLEN(drv, p);
	return (total);
}


uchar*
bcm_ether_ntoa(char *ea, char *buf)
{
	sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",
		(uchar)ea[0]&0xff, (uchar)ea[1]&0xff, (uchar)ea[2]&0xff,
		(uchar)ea[3]&0xff, (uchar)ea[4]&0xff, (uchar)ea[5]&0xff);
	return (buf);
}

/* parse a xx:xx:xx:xx:xx:xx format ethernet address */
int
bcm_ether_atoe(char *p, char *ea)
{
	int i = 0;

	for (;;) {
		ea[i++] = (char) bcm_strtoul(p, &p, 16);
		if (!*p++ || i == 6)
			break;
	}

	return (i == 6);
}

/* 
 * Advance from the current 1-byte tag/1-byte length/variable-length value 
 * triple, to the next, returning a pointer to the next.
 */
bcm_tlv_t *
bcm_next_tlv(bcm_tlv_t *elt, int *buflen)
{
	int len;

	/* validate current elt */
	if (*buflen < 2) {
		return NULL;
	}
	
	len = elt->len;

	/* validate remaining buflen */
	if (*buflen >= (2 + len + 2)) {
		elt = (bcm_tlv_t*)(elt->data + len);
		*buflen -= (2 + len);
	} else {
		elt = NULL;
	}
	
	return elt;
}

/* 
 * Traverse a string of 1-byte tag/1-byte length/variable-length value 
 * triples, returning a pointer to the substring whose first element 
 * matches tag.  Stop parsing when we see an element whose ID is greater
 * than the target key. 
 */
bcm_tlv_t *
bcm_parse_ordered_tlvs(void *buf, int buflen, uint key)
{
	bcm_tlv_t *elt;
	int totlen;

	elt = (bcm_tlv_t*)buf;
	totlen = buflen;

	/* find tagged parameter */
	while (totlen >= 2) {
		uint id = elt->id;
		int len = elt->len;
		
		/* Punt if we start seeing IDs > than target key */
		if (id > key)
			return(NULL);

		/* validate remaining totlen */
		if ((id == key) && (totlen >= (len + 2)))
			return (elt);

		elt = (bcm_tlv_t*)((uint8*)elt + (len + 2));
		totlen -= (len + 2);
	}
	return NULL;
}


/* 
 * Traverse a string of 1-byte tag/1-byte length/variable-length value 
 * triples, returning a pointer to the substring whose first element 
 * matches tag
 */
bcm_tlv_t *
bcm_parse_tlvs(void *buf, int buflen, uint key)
{
	bcm_tlv_t *elt;
	int totlen;

	elt = (bcm_tlv_t*)buf;
	totlen = buflen;

	/* find tagged parameter */
	while (totlen >= 2) {
		int len = elt->len;

		/* validate remaining totlen */
		if ((elt->id == key) && (totlen >= (len + 2)))
			return (elt);

		elt = (bcm_tlv_t*)((uint8*)elt + (len + 2));
		totlen -= (len + 2);
	}
	
	return NULL;
}

void
pktq_init(struct pktq *q, uint maxlen, bool priority)
{
	q->head = q->tail = NULL;
	q->priority = priority;
	q->maxlen = maxlen;
	q->len = 0;
}

bool
pktenq(struct pktq *q, void *p, bool lifo)
{
	void *next, *prev;

	/* Queue is full */
	if (q->len >= q->maxlen)
		return FALSE;

	/* Queueing chains not allowed */
	ASSERT(PKTLINK(p) == NULL);

	/* Queue is empty */
	if (q->tail == NULL) {
		ASSERT(q->head == NULL);
		q->head = q->tail = p;
	}

	/* Insert at head or tail */
	else if (q->priority == FALSE) {
		/* Insert at head (LIFO) */
		if (lifo) {
			PKTSETLINK(p, q->head);
			q->head = p;
		}
		/* Insert at tail (FIFO) */
		else {
			ASSERT(PKTLINK(q->tail) == NULL);
			PKTSETLINK(q->tail, p);
			PKTSETLINK(p, NULL);
			q->tail = p;
		}
	}

	/* Insert by priority */
	else {
		ASSERT(q->head);
		ASSERT(q->tail);
		/* Shortcut to insertion at tail */
		if (PKTPRIO(p) < PKTPRIO(q->tail) ||
		    (!lifo && PKTPRIO(p) <= PKTPRIO(q->tail))) {
			prev = q->tail;
			next = NULL;
		}
		/* Insert at head or in the middle */
		else {
			prev = NULL;
			next = q->head;
		}
		/* Walk the queue */
		for (; next; prev = next, next = PKTLINK(next)) {
			/* Priority queue invariant */
			ASSERT(!prev || PKTPRIO(prev) >= PKTPRIO(next));
			/* Insert at head of string of packets of same priority (LIFO) */
			if (lifo) {
				if (PKTPRIO(p) >= PKTPRIO(next))
					break;
			}
			/* Insert at tail of string of packets of same priority (FIFO) */
			else {
				if (PKTPRIO(p) > PKTPRIO(next))
					break;
			}
		}
		/* Insert at tail */
		if (next == NULL) {
			ASSERT(PKTLINK(q->tail) == NULL);
			PKTSETLINK(q->tail, p);
			PKTSETLINK(p, NULL);
			q->tail = p;
		}
		/* Insert in the middle */
		else if (prev) {
			PKTSETLINK(prev, p);
			PKTSETLINK(p, next);
		}
		/* Insert at head */
		else {
			PKTSETLINK(p, q->head);
			q->head = p;
		}
	}

	/* List invariants after insertion */
	ASSERT(q->head);
	ASSERT(PKTLINK(q->tail) == NULL);

	q->len++;
	return TRUE;
}

void*
pktdeq(struct pktq *q)
{
	void *p;

	if ((p = q->head)) {
		ASSERT(q->tail);
		q->head = PKTLINK(p);
		PKTSETLINK(p, NULL);
		q->len--;
		if (q->head == NULL)
			q->tail = NULL;
	}
	else {
		ASSERT(q->tail == NULL);
	}

	return (p);
}

/*
 * Search the name=value vars for a specific one and return its value.
 * Returns NULL if not found.
 */
char*
getvar(char *vars, char *name)
{
	char *s;
	int len;

	len = strlen(name);

	/* first look in vars[] */
	for (s = vars; s && *s; ) {
		if ((bcmp(s, name, len) == 0) && (s[len] == '='))
			return (&s[len+1]);

		while (*s++)
			;
	}
#if 0 /* alok */
	/* then query nvram */
	return nvram_get_ecos(name);
#else
	return s;
#endif
}

/*
 * Search the vars for a specific one and return its value as
 * an integer. Returns 0 if not found.
 */
int
getintvar(char *vars, char *name)
{
	char *val;

	if ((val = getvar(vars, name)) == NULL)
		return (0);

	return (bcm_strtoul(val, NULL, 0));
}

/* return pointer to location of substring 'needle' in 'haystack' */
char*
bcmstrstr(char *haystack, char *needle)
{
	int len, nlen;
	int i;

	if ((haystack == NULL) || (needle == NULL))
		return (haystack);

	nlen = strlen(needle);
	len = strlen(haystack) - nlen + 1;

	for (i = 0; i < len; i++)
		if (bcmp(needle, &haystack[i], nlen) == 0)
			return (&haystack[i]);
	return (NULL);
}

void
bcm_mdelay(uint ms)
{
	uint i;

	for (i = 0; i < ms; i++) {
		OSL_DELAY(1000);
	}
}

#define CRC_INNER_LOOP(n, c, x) \
    (c) = ((c) >> 8) ^ crc##n##_table[((c) ^ (x)) & 0xff]




