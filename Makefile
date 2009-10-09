# Makefile for flicp
# programm from 
# 
# Copyright (C) 2002 Christopher Loessl <c.loessl@gmx.net>
# Copyright (C) 2002 Stefan Strigler <steve@zeank.in-berlin.de>
#
# This programm is under the GPL 2
#
#  Created:				2002 08 03
#  Last updated:	2002 08 04


CC = gcc
CFLAGS = -ggdb -Wall


all: flicp

flicp: flicp.o

flipcp.o: flicp.c

clean:
	@rm -f *.o flicp *~

tar-ball:
	@tar cvfz flicp.tgz *.c Makefile README AUTHORS COPYING

bz2-ball:
	@tar cvfj flicp.tgz *.c Makefile README AUTHORS COPYING
