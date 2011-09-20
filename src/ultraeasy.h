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

#ifndef ULTRAEASY_H_
#define ULTRAEASY_H_

#include <stdint.h>
#include <time.h>

#include "ue_link.h"

typedef struct ultraeasy ultraeasy_t;

typedef struct ultraeasy_record {
	time_t date;
	double mmol_per_litre;

	struct {
		uint32_t date;
		uint32_t reading;
	} raw;
} ultraeasy_record_t;

ultraeasy_t *ultraeasy_open(const char *pathname);
time_t ultraeasy_read_rtc(ultraeasy_t *ultraeasy);
char *ultraeasy_read_serial(ultraeasy_t *ultraeasy);
char *ultraeasy_read_version(ultraeasy_t *ultraeasy);
int ultraeasy_num_records(ultraeasy_t *ultraeasy);
int ultraeasy_get_record(ultraeasy_t *ultraeasy, unsigned int num, ultraeasy_record_t *record);
void ultraeasy_close(ultraeasy_t *ultraeasy);

#endif /* ULTRAEASY_H_ */
