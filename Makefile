# Makefile for CPE464 program 1

CC= gcc
CFLAGS= -g

# The  -lsocket -lnsl are needed for the sockets.
# The -L/usr/ucblib -lucb gives location for the Berkeley library needed for
# the bcopy, bzero, and bcmp.  The -R/usr/ucblib tells where to load
# the runtime library.

# The next line is needed on Sun boxes (so uncomment it if your on a
# sun box)
# LIBS =  -lsocket -lnsl

# For Linux/Mac boxes uncomment the next line - the socket and nsl
# libraries are already in the link path.
LIBS = libcpe464.2.12.a -lstdc++

all:   m_printer server cclient  

m_printer: m_printer.c networks.h 
	$(CC) $(CFLAGS) -o m_printer m_printer.c -c 

cclient: cclient.c networks.h cpe464.h
	$(CC) $(CFLAGS) -o cclient cclient.c m_printer $(LIBS)

server: server.c networks.h cpe464.h
	$(CC) $(CFLAGS) -o server server.c m_printer $(LIBS)


clean:
	rm server cclient m_printer



