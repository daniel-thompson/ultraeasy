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

#ifndef UE_LINK_H_
#define UE_LINK_H_

#define LINK_MAX_MSG_LEN 34

typedef struct link_msg {
	unsigned len;
	unsigned char data[LINK_MAX_MSG_LEN];
} link_msg_t;

typedef struct link link_t;

link_t *link_open(const char *pathname);
int link_reset(link_t *link);
int link_command(link_t *link, link_msg_t *input, link_msg_t *output);
void link_close(link_t *link);

#endif // UE_LINK_H_
