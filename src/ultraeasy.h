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

#ifndef ONETOUCH_H_
#define ONETOUCH_H_

#include <stdint.h>
#include <time.h>

#include "ue_link.h"

typedef struct onetouch onetouch_t;

typedef struct onetouch_record {
	time_t date;
	double mmol_per_litre;

	struct {
		uint32_t date;
		uint32_t reading;
	} raw;
} onetouch_record_t;

onetouch_t *onetouch_open(const char *pathname);
time_t onetouch_read_rtc(onetouch_t *onetouch);
char *onetouch_read_serial(onetouch_t *onetouch);
char *onetouch_read_version(onetouch_t *onetouch);
int onetouch_num_records(onetouch_t *onetouch);
int onetouch_get_record(onetouch_t *onetouch, unsigned int num, onetouch_record_t *record);
void onetouch_close(onetouch_t *onetouch);

#endif /* ONETOUCH_H_ */
