/* addr.h: Functions for identifying and handling memory addresses.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_addr_h_
#define	_addr_h_

/* The information kept for each segment of memory.
 */
typedef	struct addrinfo {
    unsigned	addr;		/* the address of a segment */
    unsigned	from;		/* the lowest address used in this segment */
    unsigned	to;		/* the highest address used in this segment */
    int		count;		/* how many addresses are in this segment */
    int		size;		/* the largest single chunk in this segment */
    char	name[128];	/* the preprocessor name for this segment */
} addrinfo;

/* The collection of addresses.
 */
extern addrinfo		       *addrs;
extern int			addrnum;

/* Records a memory address found in the file, with its probable
 * offset within the segment, the size of the memory chunk, and the
 * associated name. This information is used to build the collection
 * of memory segments.
 */
extern void recordaddr(unsigned int addr, int offset, int size,
		       char const *name);

/* Assigns each identified memory segment a unique name. Called after
 * all addresses have been recorded.
 */
extern void setaddrnames(void);

/* Returns the segment information associated with a given address.
 */
extern addrinfo const *getbaseaddr(unsigned int addr);

#endif
