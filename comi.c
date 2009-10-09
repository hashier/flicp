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


#include "comi.h"


void usage(char *progname) {

	
	printf("Usage: %s [-v] [-h] [-p port] [-u <userpw>] [-a <adminpw>] -r <rootpw> <localfile> <host>:<remotefile>\n", progname);

	putchar('\n');

	printf("       %s [-v] [-h] [-p port] [-u <userpw>] [-a <adminpw>] -r <rootpw> <host>:<remotefile> <localfile>\n", progname);
	putchar('\n');
	printf("       host        remote machine to connect to\n");
	printf("       remotefile  path to file on remote machine\n");
	printf("       localfile   path to local file\n");
	printf("       -v          be a little bit more verbose\n");
	printf("       -p port	   alternate port on remote machine\n");
	printf("       -h          print this help\n");
	printf("       -u          set the userpw\n");
	printf("       -a          set the adminpw\n");
	printf("       -r          set the rootpw\n");
	printf("       -q          silent (quit) (stupid with -v)\n");
	printf("       -i          interactic mode (ask for passwords)\n");
	exit(1);
}


int send_buf(int fd, char *str) {
	int len=strlen(str);
	char buf[len+2];

	sprintf(buf, "%s\r\n", str);
	if (send(fd, buf, len+2, 0) < 0) {
		perror("send");
		return -1;
	}

	if (verbose)
		printf(">>> %s.\n", str);

	return 0;
}   /* unused */


/* antwort bekommen */
char *get_answer (int fd) { 
	static char buf[8192];
	char *tmp;
	char c[1];
	int len=0;
	
	tmp=buf;
	while (read(fd, c, 1) > 0) {
		len ++;
		if (c[0]=='\n')
			break; /* break at end of line */
		if (c[0] != '\r')
			*tmp++=c[0];
	}

	*tmp='\0';

	if (len <= 0)
		return ((char *) NULL);
	
	/* [zeank] scanning for newline not necessary anymore */
/* 	while (len > 1 && (buf[len - 1]=='\n' || buf[len - 1]=='\r')) { */
/* 		buf[len - 1]='\0'; */
/* 		len--; */
/* 	} */
	
#ifdef DEBUG 
		printf("<<< %s.\n", buf);
#endif

	if (!strncmp(buf, "OK ", 3)) {	/* OK xxxx */
		return (buf + 3);
	}	else if (len > 2 && !strcmp(buf + len - 2, "OK")) {
		*(buf + len - 2)='\0';
		return (buf);
	}	else if (len==2 && !strcmp(buf, "OK")) {
		return (buf);
	}
	return ((char *) NULL); /* ERR xxxx	*/
} /* get_answer (fd) */


/* for interaktiv mode */
int read_line (char *password) {
	int zaehler=0; /* counts the letters, max is 80 */
	int letter;

	letter=getchar();
	
	while (zaehler < MAX_L && letter != EOF && letter!='\n'){
		*password++=letter;
		letter=getchar();
		zaehler++;
	}
	*password='\0';
	if(letter==EOF )
		return EOF;
	else
		return zaehler;
}
