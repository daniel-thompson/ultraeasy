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

#ifndef UTIL_H_
#define UTIL_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define ERROR(...) tracefn(0, __FUNCTION__, "Error - ", __VA_ARGS__)
#define TRACE(...) tracefn(1, __FUNCTION__, "", __VA_ARGS__)
#define DEBUG(...) tracefn(2, __FUNCTION__, "", __VA_ARGS__)
extern int trace_level;
void tracefn(int level, const char *fn, const char *prefix, const char *fmt, ...);

void fatal(const char *fmt, ...);

#define MS_ERR ((uint64_t) -1)
uint64_t timespec_to_ms(const struct timespec *tp);
void ms_to_timespec(uint64_t ms, struct timespec *tp);
uint64_t ms_gettime(clockid_t clk_id);

/**
 * printf() to log file
 *
 * Appart from the void return type this function acts identically to
 * fprintf() when the argument is non-NULL. When the argument is NULL the
 * function is a no-op.
 */
void lprintf(FILE *f, const char *fmt, ...);

char *strdup_asciify(const unsigned char *p, unsigned int len);
char *xstrdup_asciify(const unsigned char *p, unsigned int len);
char *strdup_hexdump(const unsigned char *p, unsigned int len);
char *xstrdup_hexdump(const unsigned char *p, unsigned int len);

void *xmalloc(size_t size);
char *xstrdup(const char *str);

char *strdup_printf(const char *fmt, ...);
char *xstrdup_printf(const char *fmt, ...);

char *strdup_vprintf(const char *fmt, va_list ap);
char *xstrdup_vprintf(const char *fmt, va_list ap);

#endif /* UTIL_H_ */
