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
#include <stdlib.h>
#include <string.h>

#include "onetouch.h"
#include "util.h"

struct onetouch {
	link_t *link;
};

onetouch_t *onetouch_open(const char *pathname)
{
	onetouch_t *onetouch = xmalloc(sizeof(onetouch_t));

	onetouch->link = link_open(pathname);
	if (NULL == onetouch->link) {
		free(onetouch);
		return NULL;
	}

	return onetouch;;
}

char *onetouch_read_version(onetouch_t *onetouch)
{
	const unsigned char in[] = { 0x05, 0x0d, 0x02 };
	const unsigned char out[] = { 0x05, 0x06, 0x11 };

	link_msg_t cmd, reply;

	memcpy(cmd.data, in, sizeof(in));
	cmd.len = sizeof(in);

	int res = link_command(onetouch->link, &cmd, &reply);
	if (0 != res)
		return NULL;

	if (0 != memcmp(reply.data, out, sizeof(out))) {
		ERROR("Unexpected reply tag from meter\n");
		errno = EPROTO;
		return NULL;
	}

	char *version = xmalloc(reply.len);
	memcpy(version, reply.data + sizeof(out), reply.len - sizeof(out));
	version[reply.len - sizeof(out)] = '\0';

	return version;
}

char *onetouch_read_serial(onetouch_t *onetouch)
{
#if 0
	const unsigned char cmdstr[] = { 0x05, 0x0b, 0x02, 0x00, 0x00, 0x00, 0x00,
			      	     0x84,0x6a, 0xe8, 0x73, 0x00 };
#else
	const unsigned char cmdstr[] = { 0x05, 0x0b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif
	const unsigned char replystr[] = { 0x05, 0x06 };

	link_msg_t cmd, reply;

	memcpy(cmd.data, cmdstr, sizeof(cmdstr));
	cmd.len = sizeof(cmdstr);

	int res = link_command(onetouch->link, &cmd, &reply);
	if (0 != res)
		return NULL;

	if (0 != memcmp(reply.data, replystr, sizeof(replystr))) {
		ERROR("Unexpected reply tag from meter\n");
		errno = EPROTO;
		return NULL;
	}

	char *version = xmalloc(reply.len);
	memcpy(version, reply.data + sizeof(replystr), reply.len - sizeof(replystr));
	version[reply.len - sizeof(replystr)] = '\0';

	return version;
}

int onetouch_num_records(onetouch_t *onetouch)
{
	const unsigned char cmdstr[] = { 0x05, 0x1f, 0xf5, 0x01 };

	link_msg_t cmd, reply;

	memcpy(cmd.data, cmdstr, sizeof(cmdstr));
	cmd.len = sizeof(cmdstr);

	int res = link_command(onetouch->link, &cmd, &reply);
	if (0 != res)
		return NULL;

	return 0;
}


void onetouch_close(onetouch_t *onetouch)
{
	link_close(onetouch->link);
	free(onetouch);
}
