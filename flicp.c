/*  flicp - copy files to/from imond server
 * 
 *  Copyright (C) 2002 Christopher Loessl <c.loessl@gmx.net>
 *  Copyright (C) 2002 Stefan Strigler <steve@zeank.in-berlin.de>
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
 *  Created:			2002 07 27
 *  Last updated:	2002 08 05
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

#define IMOND_PORT	5000
#define MAXBUF			1025
#define PERMS	0660
#define ACK	"\006"

#define DEBUG

int verbose;

void usage(char *progname) {
	printf("Usage: %s [-v] [-h] [-p port] [-u <userpw>] [-a <adminpw>] -r <rootpw> <localfile> <host>:<remotefile>\n", progname);

	putchar('\n');

	printf("       %s [-v] [-h] [-p port] [-u <userpw>] [-a <adminpw>] -r <rootpw> <host>:<remotefile> <localfile>\n", progname);
	putchar('\n');
	printf("       host	       remote machine to connect to\n");
	printf("       remotefile  path to file on remote machine\n");
	printf("       localfile   path to local file\n");
	printf("       -v	         be a little bit more verbose\n");
	printf("       -p port	   alternate port on remote machine\n");
	printf("       -h	         print this help\n");
	printf("       -u          set the userpw\n");
	printf("       -a          set the adminpw\n");
	printf("       -r          set the rootpw\n");
	printf("       -q          silent (stupid with -v)\n");
	exit(1);
}

int send_buf(int fd, char *str) {
	int len = strlen(str);
	char buf[len+2];

	sprintf(buf, "%s\r\n", str);
	if (send(fd, buf, len+2, 0) < 0) {
		perror("send");
		return -1;
	}

	if (verbose)
		printf(">>> %s.\n", str);

	return 0;
}   /* code that never will be execude, BUT WHY? Steve what are u doing??? */

char *get_answer (int fd) { 
	static char buf[8192];
	int len;
	
	len = read(fd, buf, 8192);
	if (len <= 0)
		return ((char *) NULL);
	
	while (len > 1 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
		buf[len - 1] = '\0';
		len--;
	}
	
/* #ifdef DEBUG */
/* 		printf("<<< %s.\n", buf); */
/* 		Printf out the answer from the server */
/* #endif */

	if (!strncmp(buf, "OK ", 3)) {	/* OK xxxx */
		return (buf + 3);
	}	else if (len > 2 && !strcmp(buf + len - 2, "OK")) {
		*(buf + len - 2) = '\0';
		return (buf);
	}	else if (len == 2 && !strcmp(buf, "OK")) {
		return (buf);
	}
	return ((char *) NULL); /* ERR xxxx	*/
} /* get_answer (fd) */


/* -----===== MAIN ==== ----- */
 
int main (int argc, char *argv[]) {
	int fd, filefd;
	int port;
	int sending = 0;
	int pass_stat;
	int size=0;
	int recvBytes;
	int nowrecv;
	int sntBytes=0;
	int nowsent=0;
	int silent=0;
	char 
		*host,
		*remote_path,
		*localfile,
		*answer,
		*userpw,
		*adminpw,
		*rootpw,
		//		*buffer_tmp,
		*ptr,
		buffer[MAXBUF];

	DIR *tmpdir;
	

	struct sockaddr_in self;
	struct hostent *he; /* used by gethostbyname */
	struct stat fbuf; /* did I say "I HATE STRUCTS"? */
	


	/* stuff for getopt */
	int opt;
	extern char *optarg;
	extern int optind, opterr, optopt;
	
	userpw=NULL;
	adminpw=NULL;
	rootpw=NULL;

	port = IMOND_PORT;

	while ((opt=getopt(argc,argv,"u:a:r:p:vh?q")) > 0) {
		switch (opt) {
		case ':' :
		case 'h' :
		case '?' : usage(argv[0]); break;
		case 'v' : verbose=1; break;
		case 'p' : port=atoi(optarg); break;
		case 'u' : userpw=optarg; break;
		case 'a' : adminpw=optarg; break;
		case 'r' : rootpw=optarg; break;
		case 'q' : silent=1; break;
		}
	}

	if (argc < optind+2)
		usage(argv[0]);

	/* gettin' the information for the connect */

	host = argv[optind]; /* asume reveive mode */
	if ((remote_path=strchr(host, ':')) == NULL) { /* send mode */
		/* flicp ... <localfile> <host>:<remote_path> */
																									
		localfile = argv[optind++];
		host = argv[optind++];
		if ((remote_path=strchr(host, ':')) == NULL)
			usage(argv[0]);
		*remote_path++ = '\0';
		sending = 1;
		if (!silent) {
			putchar('\n');
			printf("userpw: %s\n", userpw);
			printf("adminpw: %s\n", adminpw);
			printf("rootpw: %s\n", rootpw);
			putchar('\n');
			printf("host: %s \n", host);
			printf("remote_path: %s\n", remote_path);
			printf("remote_file: %s\n", localfile);
		}
	} else { /* receive mode */
		/* flicp ... <host>:<remote_path> 
		 * <remotefile(variable localfile)>  */

		*remote_path++ = '\0';
		optind++;
		localfile = argv[optind++];

		if (!silent) {
			putchar('\n');
			printf("userpw: %s\n", userpw);
			printf("adminpw: %s\n", adminpw);
			printf("rootpw: %s\n", rootpw);
			putchar('\n');
			printf("host: %s\n", host);
			printf("remote_path: %s\n", remote_path);
			printf("remote_file: %s\n", localfile);
			putchar('\n');
		}
	}

	/* lookup remote host */
	he = gethostbyname(host);

	if (!he) {
		fprintf (stderr, "%s: host not found\n", host);
		return -1;
	}

	bzero(&self, sizeof(self));
	self.sin_family = PF_INET;  
	self.sin_port = htons(port);  /* only converting for intel and mac */
	self.sin_addr = *((struct in_addr*)he->h_addr);

	/* open socket */
	if ((fd=socket(PF_INET, SOCK_STREAM, 0)) < 0) { /* No sockets available */
		perror("socket");
		return -1;
	}

	/* connect to remote */
	if (connect(fd, (struct sockaddr *)&self, sizeof(self)) < 0) {
		perror("connect");
		return -1;
	}

	if (verbose)
		printf("Connected to host %s.\n", host);

	pass_stat=1;
	/* =====-- Set the passwords --===== */
	while (pass_stat < 4) {
		sprintf(buffer, "pass\n");
		send(fd, buffer, strlen(buffer), 0);
		pass_stat=atoi(answer =	get_answer(fd));
/* #ifdef DEBUG */
/* 		printf("Pass status: %d\n", pass_stat); */
/* #endif */
		if (pass_stat == 1) {
			snprintf(buffer, sizeof(buffer), "pass %s\n", userpw);
			send(fd, buffer, strlen(buffer), 0);
			pass_stat = atoi(answer =	get_answer(fd));
			if (pass_stat <= 1) {
				printf("Wrong user password\n");
				break;
			}

		} else if (pass_stat == 2) {
			snprintf(buffer, sizeof(buffer), "pass %s\n", adminpw);
			send(fd, buffer, strlen(buffer), 0);
			pass_stat = atoi(answer = get_answer(fd));
			if (pass_stat <= 2) {
				printf("Wrong admin password\n");			
				break;
			}
			
		} else if (pass_stat == 3) {
			snprintf(buffer, sizeof(buffer), "pass %s %s\n", userpw, adminpw);
			send(fd, buffer, strlen(buffer), 0);
			pass_stat = atoi(answer =	get_answer(fd));
			if (pass_stat <= 3) {
				printf("Wrong passwords\n");
				break;
			}
		}
		if (pass_stat >= 4) {
			if (!silent)
				printf("Na endlich, ready for transfering\n\n");
		}
	}



	if (!sending && (tmpdir=opendir(localfile))) { /* filename was a local directory
																			* so we must build the
																			* local filename from local
																			* path and remote path
																			*/
		char *rem_filename;

		/* split remote path on last / */
		rem_filename = strrchr(remote_path, '/');
		snprintf(buffer, sizeof(buffer), "%s%s", localfile, rem_filename);
		localfile = strdup(buffer);
		closedir(tmpdir);
	}

	/* stat on filename */
	bzero(&fbuf,sizeof(fbuf));

	stat(localfile, &fbuf);


	if ((filefd = open(localfile, O_RDWR|O_CREAT, PERMS)) < 0) {
		perror("open");
		return -1;
			
	}	else {
		/* send file to fli */
		if (!silent)
			printf("ok first try to send\n");

		if (sending) {
			if (!silent)
				printf("sending");
			size = fbuf.st_size;
			/* first send "recevie <filename> <bytes> <rootpw> */
			snprintf(buffer, sizeof(buffer), "receive %s %d %s\n", remote_path, size, rootpw);
			send(fd, buffer, strlen(buffer), 0);
			bzero(buffer, 1025);

/* 			while (size-(i*1024)>0) { */
			while (sntBytes < size) {
				/* TODO add if */
				nowsent = read(filefd, buffer, 1024);
				send(fd, buffer, nowsent, 0);
				sntBytes += nowsent;
				if (verbose) {
					printf("Sende: %d bytes to server\n", nowsent);
					printf("gesamt gesendet: %d\n", sntBytes);
				}
/* 				if ((sntBytes % 1024) == 0) */
				read(fd, buffer, 3);
			}
			printf("Done sending\nhopefully successful\n");

		} else { /* get file from fli */
			/* ask the server for the requested file */
			if (!silent)
				printf("receiving");
			snprintf(buffer, sizeof(buffer), "send %s %s\n", remote_path, rootpw);
			send(fd, buffer, strlen(buffer), 0);


			bzero(buffer, 1025);
			recvBytes = 0;
			nowrecv = recv(fd, buffer, 1024, 0); /* Get first line from imond */
			buffer[nowrecv] = '\0';
			ptr = buffer+3; /* Cut the "OK " away */
			size = atoi(ptr); /* all after the "OK " is the size */

			if (!silent) {
				printf("=== ANFANG ===\n");
				printf("Groesse des zu empfangenden files: %d\n", size);
			}

			ptr = strchr(buffer, '\n');
			*ptr++ = '\0';

			write(filefd, ptr, strlen(ptr));
			recvBytes = strlen(ptr);
			
			if (recvBytes >= size)
				send(fd, ACK, 1, 0);

			while(recvBytes < size)
				{
					nowrecv = recv(fd, buffer, 1024, 0); /* read 1024 bytes from fd */
					if (verbose) {
						printf("Bereits empfangen: %d\n", recvBytes);
						printf("Gerade empfangen: %d\n", nowrecv);
					}
					/* tomorrow I will write a if for better bugfix */ /* TODO */
					write(filefd, buffer, nowrecv); /* write the received bytes to the file */
					recvBytes += nowrecv;
					/*					if ((recvBytes % 1024) == 0) {*/ /* every 1024 Bytes we send a "ACK\n" */
					send(fd, ACK, 1, 0); 
/* 					if (verbose) */
/* 						printf("sende ACK\n"); */
		}
			if (recvBytes == size) {
				if (!silent)
					printf("\n === File successful received === \n");
			} else {
				if (!silent) {
					printf("\n === HU, error! FUCK UP ===\nsomething wrong with the counter, just try again, maybe it works now\n");
					printf("The bug often occur when %s is run often in a short time", argv[0]);
					/* stupid comment from hashier */
					if (!silent)
						printf("\nShutdown fd's\n");
					close(filefd);
					close(fd);
					if (!silent)
						printf("done\n");
					exit(1);
				}
			}
		}
	}
	if (!silent)
		printf("\nShutdown fd's\n");
	close(filefd);
	close(fd);
	if (!silent)
		printf("done\n");
		
	if (verbose)
		printf("Connection closed. Exiting ...\n");
		
	return 0;
}
