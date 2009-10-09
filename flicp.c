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
 *  Created:			2002 07 27
 *  Last updated:	2003 04 10
 */


#include "comi.h"

#define IMOND_PORT  5000
#define MAXBUF      1025
#define PERMS       0660
#define ACK         "\006"


/* -----===== MAIN ==== ----- */
int main (int argc, char *argv[]) {
	int fd, filefd,
			port,
			sending=0,
			pass_stat,
			size=0,
			recvBytes,
			nowrecv,
			sntBytes=0,
			nowsent=0,
			silent=0,
			interactive=0,
			showpw=0;
	char 
		*host,
		*remote_path,
		*localfile,
		*answer,
		*userpw,
		*adminpw,
		*rootpw,
		password[MAX_L+1],
		buffer[MAXBUF];
	DIR *tmpdir;
	struct sockaddr_in self;
	struct hostent *he; /* used by gethostbyname */
	struct stat fbuf; /* did I say "I HATE STRUCTS"? */ /* comment (1 year later) ok they are usefull */
	/* stuff for getopt */
	int opt;
	extern char *optarg; /* current option argument */
	extern int optind; /* current option index */
	extern int opterr; /* toggels getopt error messages */
	extern int optopt; /* current option character */
	
	
	userpw=NULL;
	adminpw=NULL;
	rootpw=NULL;

	port=IMOND_PORT;
	
	while ((opt=getopt(argc,argv,"u:a:r:p:vixh?qi")) != -1) {
		switch (opt) {
		case ':' :
		case 'h' :
		case '?' : 
			usage(argv[0]);
			break;
		case 'v' :
			verbose=1;
			break;
		case 'p' : 
			port=atoi(optarg);
			break;
		case 'u' :
			userpw=optarg;
			break;
		case 'a' :
			adminpw=optarg;
			break;
		case 'r' :
			rootpw=optarg;
			break;
		case 'q' :
			silent=1;
			break;
		case 'i' :
			interactive=1;
			break;
		case 'x' :
			showpw=0;
			break;
		}
	}

	if (argc < optind+2)
		usage(argv[0]);


	/* INTERACTIV MODE */
	if (interactive) {

		userpw=NULL;
		adminpw=NULL;  /* only if someone sets a -a pw and then he/she doesn't set in -i */
		rootpw=NULL;
		printf("\nIf you haven't set a pw just press enter\n\n");
		
		/* user pw */
		
		printf("Pleas enter USER PASSWORD: ");
		read_line(password);
		userpw=strdup(password);
		
		/* admin pw */
		printf("Pleas enter ADMIN PASSWORD: ");
		read_line(password);
		adminpw=strdup(password);
		
		/* root pw */
		printf("Pleas enter ROOT PASSWORD: ");
		read_line(password);
		rootpw=strdup(password);
		
		/* / INTERACTIV MODE */
	}


	/* gettin' the information for the connect */
	host=argv[optind]; /* asume reveive mode */  /* if sending == filename */
	/* Wenn in host kein : drin ist -> wird gesendet und nicht received, sprich host = filename */
	if ((remote_path=strchr(host, ':'))==NULL) { /* send mode */

		/* flicp ... <localfile> <host>:<remote_path> */																					
		localfile=argv[optind++];
		host=argv[optind++];
		if ((remote_path=strchr(host, ':'))==NULL)
			usage(argv[0]);
		*remote_path++='\0';
		sending=1;
	} else { /* receive mode */
		/* flicp ... <host>:<remote_path> <localfile>
		 * <remotefile(variable localfile)>  */

		*remote_path++='\0';
		optind++;
		localfile=argv[optind++];
	}
		if (!silent) {
			if (showpw) {
				putchar('\n');
				printf("userpw: %s\n", userpw);
				printf("adminpw: %s\n", adminpw);
				printf("rootpw: %s\n", rootpw);
			}
			putchar('\n');
			printf("host: %s \n", host);
			printf("remote_path: %s\n", remote_path);
			printf("File to send: %s\n", localfile);
		}
	/* lookup remote host */
	he=gethostbyname(host);

	if (!he) {
		fprintf (stderr, "%s: host not found\n", host);
		return -1;
	}

	bzero(&self, sizeof(self));
	self.sin_family=PF_INET;  
	self.sin_port=htons(port);  /* only converting for intel and mac */
	self.sin_addr=*((struct in_addr*)he->h_addr);

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

	/* =====-- Set the passwords --===== */
	sprintf(buffer, "pass\n");
	send(fd, buffer, strlen(buffer), 0);
	pass_stat=atoi(get_answer(fd));

	while (pass_stat < 4) {

#ifdef DEBUG
		printf("Pass status: %d\n", pass_stat);
#endif

		switch (pass_stat) {
		case 1:
			snprintf(buffer, sizeof(buffer), "pass %s\n", userpw);
			break;
		case 2:
			snprintf(buffer, sizeof(buffer), "pass %s\n", adminpw);
			break;
		case 3:
			snprintf(buffer, sizeof(buffer), "pass %s %s\n", userpw, adminpw);
			break;
		}

		send(fd, buffer, strlen(buffer), 0);
		answer=get_answer(fd);

		if (!answer) { /* error occured */
			switch (pass_stat) {
			case 1: printf("Error: user password wrong\n");
				break;
			case 2: printf("Error: admin password wrong\n");
				break;
			case 3: printf("Error: user and/or admin password wrong\n");
				break;
			}
			close(fd);
			return -1;
		} 

		pass_stat=atoi(answer);

		if (!silent && pass_stat >= 4 )
			printf("Logged in succesfully.\n\n");
	}
	
	/* =====-- END --- Set the passwords --===== */


	if (!sending && (tmpdir=opendir(localfile))) { 

		/* filename was a local directory so we must build the
		 * local filename from local path and remote path
		 */

		char *rem_filename;

		/* split remote path on last / */
		rem_filename=strrchr(remote_path, '/');
		snprintf(buffer, sizeof(buffer), "%s%s", localfile, rem_filename);
		localfile=strdup(buffer);
		closedir(tmpdir);
	}

	/* stat on filename */
	bzero(&fbuf,sizeof(fbuf));
	stat(localfile, &fbuf);

	if ((filefd=open(localfile, O_RDWR|O_CREAT, PERMS)) < 0) {
		perror("open");
		close(fd);
		return -1;
	}

	
	
	if (sending) {

		/* =====-- SENDING --===== */

		if (!silent)
			printf("Trying to send.\n");

		size=fbuf.st_size;
		/* first send "recevie <filename> <bytes> <rootpw> */
		snprintf(buffer, sizeof(buffer), "receive %s %d %s\n", remote_path, size, rootpw);
		send(fd, buffer, strlen(buffer), 0);
		
		read(fd, buffer, 3);
		if (!(strncmp(buffer, ACK, 1)==0)) {
			printf("An error occured. Aborting ...\n");
			close(filefd);
			close(fd);
			return -1;
		}

		if (!silent)
			printf("Sending ...\n");
		
		bzero(buffer, 1025);
		
		/* 			while (size-(i*1024)>0) { */
		while (sntBytes < size) {
			/* TODO add if */
			nowsent=read(filefd, buffer, 1024);
			send(fd, buffer, nowsent, 0);
			sntBytes += nowsent;
			if (verbose) {
				printf("Sent %d bytes to server\n", nowsent);
				printf("Total size: %d\n", sntBytes);
			}
			/* 				if ((sntBytes % 1024)==0) */
			read(fd, buffer, 3);
			if (!(strncmp(buffer, ACK, 1)==0)) {
				printf("An error occured. Aborting ...\n");
				close(filefd);
				close(fd);
				return -1;
			}

		}

		get_answer(fd);
		printf("Done sending.\n");
		
	} else { /* get file from fli */

		/* =====-- RECEIVING --===== */
		
		if (!silent)
			printf("Trying to receive.\n");
		
		/* ask the server for the requested file */
		snprintf(buffer, sizeof(buffer), "send %s %s\n", remote_path, rootpw);
		send(fd, buffer, strlen(buffer), 0);
		
		if (!(answer=get_answer(fd))) {
			printf("An error occured. Aborting ...\n");
			close(filefd);
			close(fd);
			return -1;
		}

		if (!silent)
			printf("Receiving ...\n");
		
		size=atoi(answer);

		bzero(buffer, 1025);
		recvBytes=0;
		
		if (!silent) {
				printf("=== START ===\n");
				printf("Bytes to receive: %d\n", size);
		}
		
		while(recvBytes < size)	{
			nowrecv=recv(fd, buffer, 1024, 0); /* read 1024 bytes from fd */
			recvBytes += nowrecv;
			if (verbose) {
				printf("Received chunk: %d\n", nowrecv);
				printf("Total size: %d\n", recvBytes);
			}
					
			/* write the received bytes to the file */

			write(filefd, buffer, nowrecv); 
			/*					if ((recvBytes % 1024)==0) {*/ /* every 1024 Bytes we send a "ACK\n" */
			send(fd, ACK, 1, 0);
		}
			
		if (recvBytes==size) {
			if (!silent)
				printf("\n === File successfully received === \n");
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

	/* all action done - clean up */
	if (!silent)
		printf("\nShutdown fd's\n");
	close(filefd);
	close(fd);
	if (!silent)
		printf("done.\n");
		
	if (verbose)
		printf("Connection closed. Exiting ...\n");
		
	return 0;
} /* END main */
