/* @(#)bitstring.h	1.6 07/03/11 J. Schilling */
/*
 * Copyright (c) 2003, 2007 J. Schilling
 *
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Vixie.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/bitstring.h,v 1.3 2003/06/13 19:40:13 phk Exp $
 */

#ifndef _BITSTRING_H
#define	_BITSTRING_H

typedef	unsigned char bitstr_t;

/* internal macros */
				/* byte of the bitstring bit is in */
#define	_bit_byte(bit) \
	((bit) >> 3)

				/* mask for the bit within its byte */
#define	_bit_mask(bit) \
	(1 << ((bit)&0x7))

/* external macros */
				/* bytes in a bitstring of nbits bits */
#define	bitstr_size(nbits) \
	(((nbits) + 7) >> 3)

				/* allocate a bitstring */
#define	bit_alloc(nbits) \
	(bitstr_t *)calloc((size_t)bitstr_size(nbits), sizeof (bitstr_t))

				/* allocate a bitstring on the stack */
#define	bit_decl(name, nbits) \
	((name)[bitstr_size(nbits)])

#ifdef	_SCHILY_SCHILY_H	/* initialize a bitstring */
#define	bit_init(name, nbits) \
	fillbytes(name, bitstr_size(nbits), '\0')
#else
#define	bit_init(name, nbits) \
	memset(name, '\0', bitstr_size(nbits))
#endif

#ifdef	_SCHILY_SCHILY_H	/* fill a bitstring */
#define	bit_fill(name, nbits) \
	fillbytes(name, bitstr_size(nbits), '\377')
#else
#define	bit_fill(name, nbits) \
	memset(name, '\377', bitstr_size(nbits))
#endif

				/* is bit N of bitstring name set? */
#define	bit_test(name, bit) \
	((name)[_bit_byte(bit)] & _bit_mask(bit))

				/* set bit N of bitstring name */
#define	bit_set(name, bit) \
	((name)[_bit_byte(bit)] |= _bit_mask(bit))

				/* clear bit N of bitstring name */
#define	bit_clear(name, bit) \
	((name)[_bit_byte(bit)] &= ~_bit_mask(bit))

				/* clear bits start ... stop in bitstring */
#define	bit_nclear(name, start, stop) do { \
	register bitstr_t *_name = (name); \
	register int _start = (start), _stop = (stop); \
	register int _startbyte = _bit_byte(_start); \
	register int _stopbyte = _bit_byte(_stop); \
	if (_startbyte == _stopbyte) { \
		_name[_startbyte] &=	((0xff >> (8 - (_start&0x7))) | \
					(0xff << ((_stop&0x7) + 1))); \
	} else { \
		_name[_startbyte] &= 0xff >> (8 - (_start&0x7)); \
		while (++_startbyte < _stopbyte) \
			_name[_startbyte] = 0; \
		_name[_stopbyte] &= 0xff << ((_stop&0x7) + 1); \
	} \
} while (0)

				/* set bits start ... stop in bitstring */
#define	bit_nset(name, start, stop) do { \
	register bitstr_t *_name = (name); \
	register int _start = (start), _stop = (stop); \
	register int _startbyte = _bit_byte(_start); \
	register int _stopbyte = _bit_byte(_stop); \
	if (_startbyte == _stopbyte) { \
		_name[_startbyte] |= ((0xff << (_start&0x7)) & \
				    (0xff >> (7 - (_stop&0x7)))); \
	} else { \
		_name[_startbyte] |= 0xff << ((_start)&0x7); \
		while (++_startbyte < _stopbyte) \
			_name[_startbyte] = 0xff; \
		_name[_stopbyte] |= 0xff >> (7 - (_stop&0x7)); \
	} \
} while (0)

				/* find first bit clear in name */
#define	bit_ffc(name, nbits, value) do { \
	register bitstr_t *_name = (name); \
	register int _byte, _nbits = (nbits); \
	register int _stopbyte = _bit_byte(_nbits - 1), _value = -1; \
	if (_nbits > 0) \
		for (_byte = 0; _byte <= _stopbyte; ++_byte) \
			if (_name[_byte] != 0xff) { \
				bitstr_t _lb; \
				_value = _byte << 3; \
				for (_lb = _name[_byte]; (_lb&0x1); \
				    ++_value, _lb >>= 1); \
				break; \
			} \
	if (_value >= nbits) \
		_value = -1; \
	*(value) = _value; \
} while (0)

				/* find first bit set in name */
#define	bit_ffs(name, nbits, value) do { \
	register bitstr_t *_name = (name); \
	register int _byte, _nbits = (nbits); \
	register int _stopbyte = _bit_byte(_nbits - 1), _value = -1; \
	if (_nbits > 0) \
		for (_byte = 0; _byte <= _stopbyte; ++_byte) \
			if (_name[_byte]) { \
				bitstr_t _lb; \
				_value = _byte << 3; \
				for (_lb = _name[_byte]; !(_lb&0x1); \
				    ++_value, _lb >>= 1); \
				break; \
			} \
	if (_value >= nbits) \
		_value = -1; \
	*(value) = _value; \
} while (0)

				/* find first bit clear in name */
#define	bit_ffcs(name, start, stop, value) do { \
	register bitstr_t *_name = (name); \
	register int _start = (start), _stop = (stop); \
	register int _byte = _bit_byte(_start); \
	register int _stopbyte = _bit_byte(_stop), _value = -1; \
	if (_stop >= _start) { \
		if ((_start&0x7) && \
		    (_name[_byte] & (0xff >> (8 - (_start&0x7)))) != \
		    (0xff >> (8 - (_start&0x7)))) { \
			bitstr_t _lb; \
			_value = _start; \
			_lb = _name[_byte]; \
			for (_lb >>= (8 - (_start&0x7)); (_lb&0x1); \
			    ++_value, _lb >>= 1); \
		} else for (; _byte <= _stopbyte; ++_byte) \
			if (_name[_byte] != 0xff) { \
				bitstr_t _lb; \
				_value = _byte << 3; \
				for (_lb = _name[_byte]; (_lb&0x1); \
				    ++_value, _lb >>= 1); \
				break; \
			} \
	} \
	if (_value > _stop) \
		_value = -1; \
	*(value) = _value; \
} while (0)

				/* find first bit set in name */
#define	bit_ffss(name, start, stop, value) do { \
	register bitstr_t *_name = (name); \
	register int _start = (start), _stop = (stop); \
	register int _byte = _bit_byte(_start); \
	register int _stopbyte = _bit_byte(_stop), _value = -1; \
	if (_stop >= _start) { \
		if ((_start&0x7) && \
		    (_name[_byte] & (0xff >> (8 - (_start&0x7))))) { \
			bitstr_t _lb; \
			_value = _start; \
			_lb = _name[_byte]; \
			for (_lb >>= (8 - (_start&0x7)); !(_lb&0x1); \
			    ++_value, _lb >>= 1); \
		} else for (; _byte <= _stopbyte; ++_byte) \
			if (_name[_byte]) { \
				bitstr_t _lb; \
				_value = _byte << 3; \
				for (_lb = _name[_byte]; !(_lb&0x1); \
				    ++_value, _lb >>= 1); \
				break; \
			} \
	} \
	if (_value > _stop) \
		_value = -1; \
	*(value) = _value; \
} while (0)

#endif /* !_BITSTRING_H */
