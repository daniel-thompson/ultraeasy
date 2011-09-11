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

#include "onetouch.h"
#include "util.h"

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
	bool want_meter_version = false;
	bool want_meter_serial = false;

	char *device = xstrdup("/dev/ttyUSB0");

	static struct option long_options[] = {
		{ "device", 1, 0, 'D' },
		{ "help", 0, 0, 'h' },
		{ "meter-version", 0, 0, 'r' },
		{ "meter-serial", 0, 0, 's' },
		{ "version", 0, 0, 'v' },
		{0, 0, 0,  0 }
	};


	while (-1 != (c = getopt_long(argc, argv, "D:hrsv", long_options, NULL))) {
		switch (c) {
		case 'D': // --device
			free(device);
			device = xstrdup(optarg);
			break;

		case 'h': // --help
			show_help();
			return 0;

		case 'r': // --meter-version
			want_meter_version = true;
			break;

		case 's': // --meter-serial
			want_meter_serial = true;
			break;

		case 'v': // --version
			show_version();
			return 0;

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

	if (want_meter_version)
		show_meter_version(meter);
	if (want_meter_serial)
		show_meter_serial(meter);

	onetouch_close(meter);
	return 0;
}
