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

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "facade.h"
#include "ue_link.h"
#include "util.h"

#define OFFSET_STX  0
#define STX 0x02

#define OFFSET_LEN  1
#define LEN_MIN 6
#define LEN_MAX (LEN_MIN + LINK_MAX_MSG_LEN)

#define OFFSET_LINK 2
#define LINK_RESERVED_MASK ((1 << 7) | (1 << 6) | (1 << 5))
#define LINK_MORE 4
#define LINK_DISCONNECT 3
#define LINK_ACKNOWLEDGE 2
#define LINK_E 1
#define LINK_S 0

#define OFFSET_MSG  3

#define OFFSET_ETX(p) (p[OFFSET_LEN] - 3)
#define ETX 0x03

#define OFFSET_CRC_LO(p) (OFFSET_ETX(p) + 1)
#define OFFSET_CRC_HI(p) (OFFSET_ETX(p) + 2)

#define LINK_DATA_TIMEOUT 10
#define LINK_PACKET_TIMEOUT 100
#define LINK_LAYER_TIMEOUT 500

// this is an approximation (true value is closer to 800) but I wanted a margin for error
#define LINK_US_PER_BYTE 1000

struct link {
	int fd;
	uint64_t last_packet;

	bool use_facade;

	bool e;
	bool s;

	unsigned char packet_buffer[64];
};

typedef struct link_meta {
	bool acknowledge;
	bool disconnect;
	bool e;
	bool s;
} link_meta_t;

const uint16_t INITIAL_CRC = 0xffff;

/**
 * Calculate the CCITT-CRC16 of the supplied frame.
 */
static uint16_t calculate_crc(uint16_t initial_crc, const unsigned char *buffer, unsigned int length)
{
	uint16_t crc = initial_crc;
	if (buffer != NULL)
	{
		for (unsigned int index = 0; index < length; index++)
		{
			crc = (uint16_t)((unsigned char)(crc >> 8) | (uint16_t)(crc << 8));
			crc ^= buffer [index];
			crc ^= (unsigned char)(crc & 0xff) >> 4;
			crc ^= (uint16_t)((uint16_t)(crc << 8) << 4);
			crc ^= (uint16_t)((uint16_t)((crc & 0xff) << 4) << 1);
		}
	}
	return (crc);
}

static void dump_packet(FILE *f, const char *desc, unsigned char *p)
{
	unsigned int len = p[OFFSET_LEN];

	if (trace_level < 3)
		return;

	char *hex = xstrdup_hexdump(p, len);
	char *ascii = xstrdup_asciify(p, len);

	fprintf(f, "%s: %s  %s (%d bytes)\n", desc, hex, ascii, len);

	free(hex);
	free(ascii);
}


static bool validate_packet(unsigned char *p)
{
	if (STX != p[OFFSET_STX]) {
		DEBUG("Bad STX\n");
		return false;
	}

	if (p[OFFSET_LEN] > LEN_MAX) {
		DEBUG("LEN out of range\n");
		return false;
	}

	if (p[OFFSET_LINK] & LINK_RESERVED_MASK) {
		DEBUG("LINK has reserved bits set\n");
		return false;
	}

	// TODO: validate the MORE bit

	if (ETX != p[OFFSET_ETX(p)]) {
		DEBUG("Bad ETX\n");
		return false;
	}

	uint16_t crc = calculate_crc(INITIAL_CRC, p, p[OFFSET_LEN]-2);
	unsigned char crclo = crc & 0xff;
	unsigned char crchi = (crc >> 8);
	if (crclo != p[OFFSET_CRC_LO(p)] || crchi != p[OFFSET_CRC_HI(p)]) {
		DEBUG("CRC failure\n");
		return false;
	}

	return true;
}

/**
 * Issue the packet from the link's pack buffer.
 */
static int tx_packet(link_t *link)
{
	unsigned char *p = link->packet_buffer;
	ssize_t remaining = p[OFFSET_LEN];
	uint32_t wire_time = ((remaining * LINK_US_PER_BYTE) + 999) / 1000;

	// wait for the guard period to expire
	while (1) {
		uint64_t now = ms_gettime(CLOCK_MONOTONIC);

		// the delta must be signed, we account for wire time during the transmit and therefore
		// it is quite legitimate for the last_packet to be in the future (due to socket buffering)
		int64_t delta = now - link->last_packet;
		if (delta >= LINK_PACKET_TIMEOUT)
			break;

		int timeout = LINK_PACKET_TIMEOUT - delta;
		DEBUG("TX guard period has not expired. Sleeping for %dms.\n", timeout);
		int res = poll(NULL, 0, timeout);
		if (0 != res) {
			TRACE("Cannot wait for TX guard period (%s)\n", strerror(errno));
			return -1;
		}
	}

	assert(validate_packet(p));
	dump_packet(stderr, "PC to meter", p);

	if (link->use_facade) {
		facade_tx_packet(p, remaining);
		link->last_packet = ms_gettime(CLOCK_MONOTONIC);
		return 0;
	}


	while (remaining > 0) {
		int res;

		res = write(link->fd, p, remaining);
		if (res < 0 && (errno != EAGAIN) && (errno != EWOULDBLOCK))
			return -1;

		if (res > 0)
			remaining -= res;
	}

	link->last_packet = ms_gettime(CLOCK_MONOTONIC) + wire_time;
	return 0;
}

static int rx_byte(link_t *link, int timeout, unsigned char *cp)
{
	struct pollfd pollee = { .fd = link->fd, .events = POLLIN };


	int res = poll(&pollee, 1, timeout);

	if (res < 0) {
		TRACE("Error handling meter device driver (%s)\n", strerror(errno));
		return -1;
	}

	if (0 == res) {
		errno = ETIMEDOUT;
		return -1;
	}

	assert(1 == res);

	res = read(link->fd, cp, 1);
	if (res < 0) {
		TRACE("Error reading from meter device driver (%s)\n", strerror(errno));
		return -1;
	}

	assert(1 == res);
	return 0;
}

/**
 * Receive a packet into the link's packet buffer.
 */
static int rx_packet(link_t *link)
{
	int res;
	int offset = 0;
	int remaining = LINK_MAX_MSG_LEN;
	uint64_t then;

	if (link->use_facade)
		return facade_rx_packet(link->packet_buffer, sizeof(link->packet_buffer));

	then = ms_gettime(CLOCK_MONOTONIC);
	res = rx_byte(link, LINK_LAYER_TIMEOUT, link->packet_buffer);
	if (0 != res) {
		if (ETIMEDOUT == res)
			ERROR("Timout waiting for meter (%ums)\n",
					(unsigned int ) (ms_gettime(CLOCK_MONOTONIC) - then));
		return -1;
	}

	if (link->packet_buffer[0] != STX) {
		ERROR("Received 0x%02x when expecting STX marker\n", link->packet_buffer[0]);
		errno = ENOLINK;
		return -1;
	}

	for (offset = 1; offset < remaining; offset++) {
		res = rx_byte(link, LINK_DATA_TIMEOUT, link->packet_buffer + offset);
		if (0 != res) {
			if (ETIMEDOUT == res)
				ERROR("Timeout receiving packet from meter\n");
			return -1;
		}

		if (OFFSET_LEN == offset) {
			if (link->packet_buffer[OFFSET_LEN] > LINK_MAX_MSG_LEN) {
				ERROR("Received oversized packet");
				errno = ENOLINK;
				return -1;
			}

			remaining = link->packet_buffer[OFFSET_LEN];
		}
	}

	char *hex = xstrdup_hexdump(link->packet_buffer, offset);
	DEBUG("Received %d bytes: %s\n", offset, hex);
	free(hex);

	link->last_packet = ms_gettime(CLOCK_MONOTONIC);
	return 0;
}

/**
 * Pack the meta-data and message into the link's packet buffer.
 */
static void pack_packet(link_t *link, link_meta_t meta, const link_msg_t *msg)
{
	const link_msg_t default_msg = { 0 };

	unsigned char *p = link->packet_buffer;
	if (!msg)
		msg = &default_msg;

	assert(msg->len < LINK_MAX_MSG_LEN);

	p[OFFSET_STX] = STX;
	p[OFFSET_LEN] = LEN_MIN + msg->len;
	p[OFFSET_LINK] = (meta.disconnect << LINK_DISCONNECT) |
			 (meta.acknowledge << LINK_ACKNOWLEDGE) |
			 (link->e << LINK_E) | (link->s << LINK_S);
	memcpy(p + OFFSET_MSG, msg->data, msg->len);
	p[OFFSET_ETX(p)] = ETX;

	uint16_t crc = calculate_crc(INITIAL_CRC, p, p[OFFSET_LEN]-2);
	p[OFFSET_CRC_LO(p)] = crc & 0xff;
	p[OFFSET_CRC_HI(p)] = crc >> 8;
}

/**
 * Unpack the meta-data and message from the link's packet buffer.
 */
static int unpack_packet(link_t *link, link_meta_t *meta, link_msg_t *msg)
{
	link_msg_t empty_message = { .len = 0 };

	unsigned char *p = link->packet_buffer;
	if (!msg)
		msg = &empty_message;

	if (p[OFFSET_LEN] <= LEN_MAX)
		dump_packet(stderr, "Meter to PC", p);

	if (!validate_packet(p)) {
		ERROR("Packet received from meter is corrupt\n");
		errno = ENOLINK;
		return -1;
	}

	meta->disconnect = p[OFFSET_LINK] & (1 << LINK_DISCONNECT);
	meta->acknowledge = p[OFFSET_LINK] & (1 << LINK_ACKNOWLEDGE);
	meta->e = p[OFFSET_LINK] & (1 << LINK_E);
	meta->s = p[OFFSET_LINK] & (1 << LINK_S);

	if (meta->s != link->e) {
		ERROR("Packet sequence number is incorrect\n");
		errno = ENOLINK;
		return -1;
	}

	msg->len = p[OFFSET_LEN] - LEN_MIN;
	memcpy(msg->data, p + OFFSET_MSG, msg->len);

	if (0 != empty_message.len) {
		TRACE("Received data when expecting empty packet\n");
		errno = ENOLINK;
		return -1;
	}

	return 0;
}

static int pack_and_tx(link_t *link, link_meta_t meta, const link_msg_t *msg)
{
	int res;

	pack_packet(link, meta, msg);

	res = tx_packet(link);
	if (res < 0) {
		TRACE("Cannot issue reset packet (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int rx_and_unpack(link_t *link, link_meta_t *meta, link_msg_t *msg)
{
	int res;

	res = rx_packet(link);
	if (res < 0) {
		TRACE("Cannot accept reply from meter (%s)\n", strerror(errno));
		return -1;
	}

	res = unpack_packet(link, meta, msg);
	if (res < 0) {
		TRACE("Bad back from meter (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

int do_reset(link_t *link, bool flush)
{
	link_meta_t disconnect = { .disconnect = true };
	link_meta_t acknowledge;
	int res;

	DEBUG("Attempting to link level reset\n");

	if (flush && !link->use_facade) {
		unsigned char flush_buffer[64];

		// wait for two guard periods for any stale data to arrive
		res = poll(NULL, 0, 2 * LINK_PACKET_TIMEOUT);
		if (0 != res)
			return -1;

		// clear out any stale data
		struct pollfd pollee = { .fd = link->fd, .events = POLLIN };
		while (1 == (res = poll(&pollee, 1, 0))) {
			res = read(link->fd, flush_buffer, sizeof(flush_buffer));
			assert(0 != res); // poll said data was available
			if (res < 0)
				break;

			DEBUG("Throwing away %d bytes of junk data\n", res);
		}
		if (res < 0)
			return -1;
	}

	link->e = false;
	link->s = false;

	res = pack_and_tx(link, disconnect, NULL);
	if (0 != res)
		return -1;

	res = rx_and_unpack(link, &acknowledge, NULL);
	if (0 != res)
		return 1; // non-fatal

	if (!acknowledge.acknowledge || !acknowledge.disconnect) {
		TRACE("No acknowledgement from meter\n");
		errno = ENOLINK;
		return 1; // non-fatal
	}

	return 0;
}

int do_command(link_t *link, link_msg_t *input, link_msg_t *output)
{
	int res;
	link_meta_t pc_cmd = { false };
	link_meta_t meter_ack;
	link_meta_t meter_reply;
	link_meta_t pc_ack = { .acknowledge = true };

	res = pack_and_tx(link, pc_cmd, input);
	if (0 != res)
		return -1;

	res = rx_and_unpack(link, &meter_ack, NULL);
	if (0 != res)
		return 1; // non-fatal

	if (!meter_ack.acknowledge || meter_ack.disconnect) {
		if (!meter_ack.acknowledge)
			TRACE("No acknowledgement from meter\n");
		if (meter_ack.disconnect)
			TRACE("Meter has requested disconnection\n");
		errno = ENOLINK;
		return 1; // non-fatal
	}

	// update the sequence number (this is triggered by the ACK)
	link->s = !link->s;

	res = rx_and_unpack(link, &meter_reply, output);
	if (0 != res)
		return 1; // non-fatal

	if (meter_reply.acknowledge || meter_reply.disconnect) {
		if (meter_reply.acknowledge)
			TRACE("Spurious acknowledgement from meter\n");
		if (meter_reply.disconnect)
			TRACE("Meter has requested disconnection\n");
		errno = ENOLINK;
		return 1; // non-fatal
	}

	// set the expected sequence number for the next packet
	link->e = !meter_reply.s;

	return pack_and_tx(link, pc_ack, NULL);
}

/**
 * Setup the serial port as required by the meter (8600 8n1 w/out flow control)
 */
static int configure_termios(link_t *link)
{
	int res;
	struct termios options;

	res = tcgetattr(link->fd, &options);
	if (0 != res) {
		ERROR("Device does not accept terminal control (%s)\n", strerror(errno));
		return -1;
	}

	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);

	// force raw mode
        options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        options.c_oflag &= ~OPOST;
        options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        options.c_cflag &= ~(CSIZE | PARENB);

        // local, enable receiver, 8-bit data
        options.c_cflag |= (CLOCAL | CREAD | CS8);

	res = tcsetattr(link->fd, TCSANOW, &options);
	if (0 != res) {
		ERROR("Cannot configure device parameters (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

link_t *link_open(const char *pathname)
{
	link_t *link;
	int res;

	link = xzalloc(sizeof(link_t));

	if (0 != strcmp(pathname, "facade")) {
		link->fd = open(pathname, O_RDWR);
		if (link->fd < 0)
			goto handle_error;

		res = configure_termios(link);
		if (0 != res)
			goto handle_error;
	} else {
		link->fd = -1;
		link->use_facade = true;
	}

	res = link_reset(link);
	if (0 != res)
		goto handle_error;

	return link;

    handle_error:
    	link_close(link);
        return NULL;
}

int link_reset(link_t *link)
{
	int res, retries;

	// reset gets an extra retry because the first time through we don't flush
	// stale data!
	for (retries=0; retries<4; retries++) {
		bool flush = (0 != retries);
		res = do_reset(link, flush);

		// do_command is tri-state, both -1 (fatal) and 0 (success) should
		// be passed up the stack.
		if (res <= 0)
			return res;

		// a recoverable error has occurred
		TRACE("Recoverable error during reset (%s). Retrying...\n", strerror(errno));
		assert(1 == res);
	}

	TRACE("Giving up after %d retries\n", retries);
	errno = ENOLINK;
	return -1;
}


int link_command(link_t *link, link_msg_t *input, link_msg_t *output)
{
	int res, retries;

	for (retries=0; retries<3; retries++) {
		res = do_command(link, input, output);

		// do_command is tri-state, both -1 (fatal) and 0 (success) should
		// be passed up the stack.
		if (res <= 0)
			return res;

		// a recoverable error has occurred
		TRACE("Recoverable error during command processing (%s). Retrying...\n", strerror(errno));
		assert(1 == res);
		res = link_reset(link);
		if (0 != res)
			return res;
	}

	DEBUG("Giving up after %d retries\n", retries);
	errno = ENOLINK;
	return -1;
}

void link_close(link_t *link)
{
    	if (link->fd >= 0)
    		(void) close(link->fd);
        free(link);
}

