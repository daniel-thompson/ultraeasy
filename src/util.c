/*
 * Driver for Lifescan OneTouch UltraEasy
 * Copyright (C) 2011 Daniel Thompson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

int trace_level = 0;

void tracefn(int level, const char *fn, const char *prefix, const char *fmt, ...)
{
	va_list ap;

	if (level > trace_level)
		return;

	if (trace_level <= 2)
		fn = "onetouch";

	fprintf(stderr, "%s: %s", fn, prefix);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	if (fmt[strlen(fmt)] != '\n')
		fprintf(stderr, "\n");
	va_end(ap);

	abort();
}

uint64_t timespec_to_ms(const struct timespec *tp)
{
	uint64_t ms;


	ms = 1000 * tp->tv_sec;
	assert((ms / 1000) == tp->tv_sec);
	ms += tp->tv_nsec / 1000000;

	//DEBUG("tv_sec %d  tv_nsec %d  ms %llu\n", (int) tp->tv_sec, (int) tp->tv_nsec, ms);

	return ms;
}

void ms_to_timespec(uint64_t ms, struct timespec *tp)
{
	tp->tv_sec = ms / 1000;
	tp->tv_nsec = (ms % 1000) * 1000000;
}

uint64_t ms_gettime(clockid_t clk_id)
{
	struct timespec ts;
	int res = clock_gettime(clk_id, &ts);
	if (0 != res) {
		DEBUG("Clock %d is not available (%s)\n", clk_id, strerror(errno));
		return MS_ERR;
	}

	return timespec_to_ms(&ts);
}

char *strdup_asciify(const unsigned char *p, unsigned int len)
{
	char *s, *ret;

	s = malloc(len + 1);
	if (NULL == s)
		return NULL;

	ret = s;

	for (int i=0; i<len; i++) {
		if (isprint(p[i]))
			*s++ = p[i];
		else
			*s++ = '.';
	}

	*s = '\0';

	return ret;
}

char *xstrdup_asciify(const unsigned char *p, unsigned int len)
{
	char *s = strdup_asciify(p, len);
	if (NULL == s)
		fatal("Out of memory");
	return s;
}

char *strdup_hexdump(const unsigned char *p, unsigned int len)
{
	const char lookup[] = "0123456789abcdef";
	char *s, *ret;

	/* For every 4 bytes in len we will require 9 characters to display it.
	 * We also need a terminating byte and to account for rounding errors.
	 */
	s = malloc(((len * 9) / 4) + 2);
	if (NULL == s)
		return NULL;

	ret = s;

	for (int i=0; i<len; i++) {
		if (i && 0 == (i % 4))
			*s++ = ' ';

		*s++ = lookup[p[i] >> 4];
		*s++ = lookup[p[i] & 0x0f];
	}

	*s = '\0';

	return ret;
}

char *xstrdup_hexdump(const unsigned char *p, unsigned int len)
{
	char *s = strdup_hexdump(p, len);
	if (NULL == s)
		fatal("Out of memory");
	return s;
}


void lprintf(FILE *f, const char *fmt, ...)
{
	if (f) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(f, fmt, ap);
		va_end(ap);
	}
}

void *xzalloc(size_t size)
{
	void *m = calloc(1, size);
	if (NULL == m)
		fatal("Out of memory");
	return m;
}

char *xstrdup(const char *str)
{
	char *s = strdup(str);
	if (NULL == s)
		fatal("Out of memory");
	return s;
}

char *strdup_printf(const char *fmt, ...)
{
	va_list ap;
	char *s;

	va_start(ap, fmt);
	s = strdup_vprintf(fmt, ap);
	va_end(ap);

	return s;
}

char *xstrdup_printf(const char *fmt, ...)
{
	va_list ap;
	char *s;

	va_start(ap, fmt);
	s = strdup_vprintf(fmt, ap);
	va_end(ap);

	if (NULL == s)
		fatal("Out of memory");

	return s;
}


char *strdup_vprintf(const char *fmt, va_list ap)
{
	va_list cp;
	int len;
	char *s;

	va_copy(ap, cp);
	len = vsnprintf(NULL, 0, fmt, cp);
	assert(len >= 0);
	if (len < 0)
		return NULL;
	va_end(cp);

	s = malloc(len+1);
	if (NULL == s)
		return NULL;

	vsprintf(s, fmt, ap);
	return s;
}

char *xstrdup_vprintf(const char *fmt, va_list ap)
{
	char *s = strdup_vprintf(fmt, ap);
	if (NULL == s)
		fatal("Out of memory");
	return s;
}
