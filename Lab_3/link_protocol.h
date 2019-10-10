#ifndef _LINK_PROTOCOL_H
#define _LINK_PROTOCOL_H

#include "defines.h"

//protocol functions
int llopen(int port, int flag);
int llwrite(int fd, char *buffer, int length);
int llread(int fd, char *buffer);
int llclose(int fd);

//helper functions
int send_SET();
unsigned char bcc_calc(unsigned char a, unsigned char c);

#endif 