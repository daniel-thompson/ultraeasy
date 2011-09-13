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
#include <stdlib.h>
#include <string.h>

#include "onetouch.h"
#include "util.h"

struct onetouch {
	link_t *link;
};

static int do_command(onetouch_t *onetouch,
		      const unsigned char *cmdstr, unsigned int cmdlen,
		      const unsigned char *replystr, unsigned int replylen,
		      unsigned int expectedlen, link_msg_t *reply)
{
	link_msg_t cmd;
	int res;

	memcpy(cmd.data, cmdstr, cmdlen);
	cmd.len = cmdlen;

	res = link_command(onetouch->link, &cmd, reply);
	if (0 != res)
		return res;

	if (reply->len < replylen) {
		ERROR("Reply from meter is too short\n");
		errno = EPROTO;
		return -1;
	}

	if (0 != memcmp(reply->data, replystr, replylen)) {
		ERROR("Unexpected reply tag from meter\n");
		errno = EPROTO;
		return -1;
	}

	if (0 != expectedlen) {
		if (expectedlen != reply->len) {
			ERROR("Expected %d byte reply but got %d bytes\n", expectedlen, reply->len);
			errno = EPROTO;
			return -1;
		}
	}

	DEBUG("Good reply from meter\n");
	return 0;
}

static uint16_t get_u16(unsigned char *p)
{
	return (p[1] << 8) | p[0];
}

static uint32_t get_u32(unsigned char *p)
{
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

onetouch_t *onetouch_open(const char *pathname)
{
	onetouch_t *onetouch = xzalloc(sizeof(onetouch_t));

	onetouch->link = link_open(pathname);
	if (NULL == onetouch->link) {
		free(onetouch);
		return NULL;
	}

	// Force link layer to set E & S. This is a hack to account for the fact
	// that some of the facade data is captured from a real meter and therefore
	// the facade only react if E & S are set. The opposite case (when facade
	// expect E & S are clear) is no problem because the timeout and reset
	// logic will automatically causes the bits to be cleared.
	//
	// Eventually we'll get the facade to understand E & S bits and then
	// we can remove this hack.
	if (0 == strcmp(pathname, "facade"))
		free(onetouch_read_serial(onetouch));

	return onetouch;;
}

time_t onetouch_read_rtc(onetouch_t *onetouch)
{
	const unsigned char cmdstr[] = { 0x05, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00 };
	const unsigned char replystr[] = { 0x05, 0x06 };

	link_msg_t reply;

	int res = do_command(onetouch, cmdstr, sizeof(cmdstr), replystr, sizeof(replystr), 6, &reply);
	if (0 != res)
		return (time_t) -1;

	return get_u32(reply.data + 2);
}

char *onetouch_read_version(onetouch_t *onetouch)
{
	const unsigned char cmdstr[] = { 0x05, 0x0d, 0x02 };
	const unsigned char replystr[] = { 0x05, 0x06, 0x11 };

	link_msg_t reply;

	int res = do_command(onetouch, cmdstr, sizeof(cmdstr), replystr, sizeof(replystr), 0, &reply);
	if (0 != res)
		return NULL;

	char *version = xzalloc(reply.len);
	memcpy(version, reply.data + sizeof(replystr), reply.len - sizeof(replystr));
	version[reply.len - sizeof(replystr)] = '\0';

	return version;
}

char *onetouch_read_serial(onetouch_t *onetouch)
{
#if 1
	const unsigned char cmdstr[] = { 0x05, 0x0b, 0x02, 0x00, 0x00, 0x00, 0x00,
			      	     0x84,0x6a, 0xe8, 0x73, 0x00 };
#else
	const unsigned char cmdstr[] = { 0x05, 0x0b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif
	const unsigned char replystr[] = { 0x05, 0x06 };

	link_msg_t reply;

	int res = do_command(onetouch, cmdstr, sizeof(cmdstr), replystr, sizeof(replystr), 0, &reply);
	if (0 != res)
		return NULL;

	char *version = xzalloc(reply.len);
	memcpy(version, reply.data + sizeof(replystr), reply.len - sizeof(replystr));
	version[reply.len - sizeof(replystr)] = '\0';

	return version;
}

int onetouch_num_records(onetouch_t *onetouch)
{
#if 0
	const unsigned char cmdstr[] = { 0x05, 0x1f, 0xf5, 0x01 };
#else
	const unsigned char cmdstr[] = { 0x05, 0x1f, 0x00, 0x02 };
#endif
	const unsigned char replystr[] = { 0x05, 0x0f };
	link_msg_t reply;

	int res = do_command(onetouch, cmdstr, sizeof(cmdstr), replystr, sizeof(replystr), 4, &reply);
	if (0 != res)
		return -1;

	return get_u16(reply.data + 2);
}

int onetouch_get_record(onetouch_t *onetouch, unsigned int num, onetouch_record_t *record)
{
	unsigned char cmdstr[] = { 0x05, 0x1f, 0x00, 0x00 };
	const unsigned char replystr[] = { 0x05, 0x06 };

	link_msg_t reply;

	assert(num < 500);

	cmdstr[2] = num & 0xff;
	cmdstr[3] = (num >> 8) & 0xff;

	int res = do_command(onetouch, cmdstr, sizeof(cmdstr), replystr, sizeof(replystr), 10, &reply);
	if (0 != res)
		return -1;

	record->raw.date = get_u32(reply.data + 2);
	record->raw.reading = get_u32(reply.data + 6);

	record->date = record->raw.date;
	record->mmol_per_litre = (double) record->raw.reading / 18.0;

	return 0;
}

void onetouch_close(onetouch_t *onetouch)
{
	link_close(onetouch->link);
	free(onetouch);
}
