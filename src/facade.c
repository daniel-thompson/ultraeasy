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

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "facade.h"
#include "util.h"

typedef struct {
	unsigned char *p;
	unsigned int len;
} facade_atom_t;

typedef struct {
	facade_atom_t key;
	facade_atom_t packets[3];
} facade_data_t;

static facade_atom_t *next_packet;

static unsigned char generic_ack[] = { 0x02, 0x06, 0x06, 0x03, 0xCD, 0x41 };

static unsigned char reset_key[] = { 0x02, 0x06, 0x08, 0x03, 0xc2, 0x62 };
static unsigned char reset_ack[] = { 0x02, 0x06, 0x0C, 0x03, 0x06, 0xae };

static unsigned char version_key[] = { 0x02, 0x09, 0x00, 0x05, 0x0D, 0x02, 0x03, 0xDA, 0x71 };
static unsigned char version_reply[] = { 0x02, 0x1A, 0x02, 0x05, 0x06, 0x11, 0x50, 0x30, 0x32, 0x2E, 0x30,
		                         0x30, 0x2E, 0x30, 0x30, 0x32, 0x35, 0x2F, 0x30, 0x35, 0x2F, 0x30,
		                         0x37, 0x03, 0xAB, 0x25 };

static unsigned char serial_key[] = { 0x02, 0x12, 0x00, 0x05, 0x0b, 0x02, 0x00, 0x00, 0x00, 0x00,
				      0x84,0x6a, 0xe8, 0x73, 0x00, 0x03, 0x9b, 0xea };
static unsigned char serial_reply[] = { 0x02, 0x11, 0x02, 0x05, 0x06, 0x43, 0x31, 0x37, 0x36, 0x53,
					0x41, 0x30, 0x4F, 0x30, 0x03, 0x49, 0x43 };

facade_data_t default_facade[] = {
#define TUPLE(x, y, z) { { x, sizeof(x) }, { { y, sizeof(y) }, { z, sizeof(z) }, { NULL } } }
	TUPLE(reset_key, reset_ack, reset_key),
	TUPLE(version_key, generic_ack, version_reply),
	TUPLE(serial_key, generic_ack, serial_reply),
#undef TUPLE
	{ { NULL } }
};

void facade_tx_packet(unsigned char *p, unsigned int len)
{
	next_packet = NULL;

	for(facade_data_t *f = default_facade; f->key.p; f++) {
		if ((f->key.len == len) && (0 == memcmp(f->key.p, p, len))) {
			next_packet = f->packets;

			char *tx = xstrdup_hexdump(p, len);
			DEBUG("Received recognised packet (%s)\n", tx);
			free(tx);
			break;
		}
	}
}

int facade_rx_packet(unsigned char *p, unsigned int len)
{
	if (!next_packet) {
		DEBUG("No packet availabe\n");
		errno = ENOLINK;
		return -1;
	}

	assert(next_packet->len <= len);
	memcpy(p, next_packet->p, next_packet->len);

	char *rx = xstrdup_hexdump(p, next_packet->len);
	DEBUG("Sending packet (%s)\n", rx);
	free(rx);

	next_packet++;
	return 0;
}
