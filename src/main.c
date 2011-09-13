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

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "onetouch.h"
#include "util.h"

typedef void (*foreach_reading_t)(void *, onetouch_record_t *);

static int foreach_reading(onetouch_t *meter, foreach_reading_t fn, void *ctx)
{
	int n = onetouch_num_records(meter);
	if (n < 0) {
		fprintf(stderr, "Cannot read number of records: %s\n", strerror(errno));
		return -1;
	}

	for (int i=0; i<n; i++) {
		onetouch_record_t record;
		int res = onetouch_get_record(meter, i, &record);
		if (res < 0) {
			fprintf(stderr, "Cannot read record %d: %s\n", i, strerror(errno));
			return -1;
		}

		fn(ctx, &record);
	}

	return 0;
}

static void show_raw_reading(void *ctx, onetouch_record_t *reading)
{
	printf("Raw date 0x%08x   Raw reading 0x%08x\n",
			reading->raw.date, reading->raw.reading);
}

static void show_reading(void *ctx, onetouch_record_t *reading)
{
	struct tm exploded;
	gmtime_r(&reading->date, &exploded);

	printf("%4d-%02d-%02d %02d:%02d:%02d    %4.1f mmol/l\n",
			exploded.tm_year + 1900, exploded.tm_mon+1, exploded.tm_mday,
			exploded.tm_hour, exploded.tm_min, exploded.tm_sec,
			reading->mmol_per_litre);
}

static void show_csv_reading(void *ctx, onetouch_record_t *reading)
{
	struct tm exploded;
	gmtime_r(&reading->date, &exploded);

	printf("\"%02d-%02d-%04d\", \"%02d:%02d:%02d\", \"%3.1f\"\n",
				exploded.tm_mday, exploded.tm_mon+1, exploded.tm_year + 1900,
				exploded.tm_hour, exploded.tm_min, exploded.tm_sec,
				reading->mmol_per_litre);
}

static void show_meter_rtc(onetouch_t *meter)
{
	time_t local, rtc;

	local = time(NULL);
	rtc = onetouch_read_rtc(meter);
	if ((time_t) -1 == rtc) {
		fprintf(stderr, "Cannot read meter real time clock: %s\n", strerror(errno));
	}

	printf("Meter time: 0x%08llx (local 0x%08llx  delta %lld)\n",
			(long long) rtc, (long long) local, (long long) (local - rtc));
}

static void show_meter_version(onetouch_t *meter)
{
	char *version;

	version = onetouch_read_version(meter);
	if (NULL == version) {
		fprintf(stderr, "Cannot read meter version number: %s\n", strerror(errno));
		version = xstrdup("(error)");
	}

	printf("Meter version: %s\n", version);
	free(version);
}

static void show_meter_serial(onetouch_t *meter)
{
	char *serial;

	serial = onetouch_read_serial(meter);
	if (NULL == serial) {
		fprintf(stderr, "Cannot read meter serial number: %s\n", strerror(errno));
		serial = xstrdup("(error)");
	}

	printf("Meter serial: %s\n", serial);
	free(serial);
}

static void show_usage()
{
	fprintf(stderr, "USAGE: onetouch [OPTION]...\n");
}

static void show_version()
{
	fprintf(stderr, "onetouch v0.1\n");
}

static void show_help()
{
	show_usage();
	show_version();
}


int main(int argc, char *argv[])
{
	int c;

	bool bad_args = false;

	char *device = "/dev/ttyUSB0";
	foreach_reading_t dumpfn = NULL;
	bool want_meter_time = false;
	bool want_meter_serial = false;
	bool want_meter_version = false;

	static struct option long_options[] = {
		{ "csv", 0, 0, 'c' },
		{ "device", 1, 0, 'D' },
		{ "dump", 0, 0, 'd' },
		{ "help", 0, 0, 'h' },
		{ "meter-time", 0, 0, 't' },
		{ "meter-serial", 0, 0, 's' },
		{ "meter-version", 0, 0, 'r' },
		{ "raw", 0, 0, 'R' },
		{ "verbose", 0, 0, 'V' },
		{ "version", 0, 0, 'v' },
		{0, 0, 0,  0 }
	};


	while (-1 != (c = getopt_long(argc, argv, "cD:dhrsVvZ", long_options, NULL))) {
		switch (c) {
		case 'c': // --csv
			dumpfn = show_csv_reading;
			break;

		case 'D': // --device
			device = optarg;
			break;

		case 'd': // --dump
			dumpfn = show_reading;
			break;

		case 'h': // --help
			show_help();
			return 0;

		case 't': // --meter-time
			want_meter_time = true;
			break;

		case 's': // --meter-serial
			want_meter_serial = true;
			break;

		case 'r': // --meter-version
			want_meter_version = true;
			break;

		case 'R': // --raw
			dumpfn = show_raw_reading;
			break;

		case 'V': // --verbose
			trace_level++;
			break;

		case 'v': // --version
			show_version();
			return 0;

		case 'Z': // no long opt
			trace_level = 3;
			break;

		default:
			bad_args = true;
		}
	}

	if (bad_args || optind < argc) {
		show_usage();
		return 1;
	}

	onetouch_t *meter;

	meter = onetouch_open(device);
	if (NULL == meter) {
		fprintf(stderr, "Cannot connect to meter: %s\n", strerror(errno));
		return 10;
	}

	if (want_meter_serial)
		show_meter_serial(meter);
	if (want_meter_version)
		show_meter_version(meter);
	if (want_meter_time)
		show_meter_rtc(meter);

	if (dumpfn) {
		int res = foreach_reading(meter, dumpfn, NULL);
		if (0 != res)
			return 12;
	}

	onetouch_close(meter);
	return 0;
}
