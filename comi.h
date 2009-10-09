/*  flicp - copy files to/from imond server
 * 
 *  Copyright (C) 2002 Christopher Loessl <cloessl@gmx.net>
 *                     Stefan Strigler <steve@zeank.in-berlin.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boson, MA 02111-1307, USA.
 *
 *  Created:			2003 04 10
 *  Last updated:	2003 04 10
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_L       80
#define VERBOSE_PRINTF(text) if(verbose) printf(text)
#define PRINTF(text2) printf(text2)

/* #define DEBUG */


void   usage        (char *);
int    send_buf     (int, char *);
char   *get_answer  (int);
int    read_line    (char *);

int verbose;
