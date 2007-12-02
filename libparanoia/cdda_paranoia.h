/* @(#)cdda_paranoia.h	1.24 07/07/28 J. Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 */

#ifndef	_CDROM_PARANOIA_H
#define	_CDROM_PARANOIA_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifndef	__GNUC__
#define	inline
#endif

#define	CD_FRAMESIZE_RAW		2352
#define	CD_FRAMEWORDS			(CD_FRAMESIZE_RAW/2)

/*
 * Second parameter of the callback function
 */
#define	PARANOIA_CB_READ		 0	/* Read off adjust ??? */
#define	PARANOIA_CB_VERIFY		 1	/* Verifying jitter */
#define	PARANOIA_CB_FIXUP_EDGE		 2	/* Fixed edge jitter */
#define	PARANOIA_CB_FIXUP_ATOM		 3	/* Fixed atom jitter */
#define	PARANOIA_CB_SCRATCH		 4	/* Unsupported */
#define	PARANOIA_CB_REPAIR		 5	/* Unsupported */
#define	PARANOIA_CB_SKIP		 6	/* Skip exhausted retry */
#define	PARANOIA_CB_DRIFT		 7	/* Drift detected */
#define	PARANOIA_CB_BACKOFF		 8	/* Unsupported */
#define	PARANOIA_CB_OVERLAP		 9	/* Dyn Overlap adjust */
#define	PARANOIA_CB_FIXUP_DROPPED	10	/* Fixed dropped bytes */
#define	PARANOIA_CB_FIXUP_DUPED		11	/* Fixed duplicate bytes */
#define	PARANOIA_CB_READERR		12	/* Hard read error */

/*
 * Cdparanoia modes to be set with paranoia_modeset()
 */
#define	PARANOIA_MODE_FULL		 0xFF
#define	PARANOIA_MODE_DISABLE		 0

#define	PARANOIA_MODE_VERIFY		 1	/* Verify data integrity in overlap area */
#define	PARANOIA_MODE_FRAGMENT		 2	/* unsupported */
#define	PARANOIA_MODE_OVERLAP		 4	/* Perform overlapped reads */
#define	PARANOIA_MODE_SCRATCH		 8	/* unsupported */
#define	PARANOIA_MODE_REPAIR		16	/* unsupported */
#define	PARANOIA_MODE_NEVERSKIP		32	/* Do not skip failed reads (retry maxretries) */


#ifndef	CDP_COMPILE
typedef	void    cdrom_paranoia;
#endif

/*
 * The interface from libcdparanoia to the high level caller
 */
extern cdrom_paranoia *paranoia_init	__PR((void * d, int nsectors,
			long	(*d_read)	__PR((void *d, void *buffer,
							long beginsector,
							long sectors)),
			long	(*d_disc_firstsector)	__PR((void *d)),
			long	(*d_disc_lastsector)	__PR((void *d)),
			int	(*d_tracks)		__PR((void *d)),
			long	(*d_track_firstsector) __PR((void *d, int track)),
			long	(*d_track_lastsector)  __PR((void *d, int track)),
			int 	(*d_sector_gettrack) __PR((void *d, long sector)),
			int 	(*d_track_audiop) __PR((void *d, int track))));

extern void	paranoia_dynoverlapset	__PR((cdrom_paranoia * p,
							int minoverlap,
							int maxoverlap));
extern void	paranoia_modeset	__PR((cdrom_paranoia * p, int mode));
extern long	paranoia_seek		__PR((cdrom_paranoia * p, long seek, int mode));
extern Int16_t	*paranoia_read		__PR((cdrom_paranoia * p, void (*callback) (long, int)));
extern Int16_t	*paranoia_read_limited	__PR((cdrom_paranoia * p, void (*callback) (long, int), int maxretries));
extern void	paranoia_free		__PR((cdrom_paranoia * p));
extern void	paranoia_overlapset	__PR((cdrom_paranoia * p, long overlap));

#ifndef	HAVE_MEMMOVE
#define	memmove(dst, src, size)		movebytes((src), (dst), (size))
#endif

#endif	/* _CDROM_PARANOIA_H */
