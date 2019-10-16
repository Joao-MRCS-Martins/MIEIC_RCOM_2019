#ifndef _LINK_PROTOCOL_H
#define _LINK_PROTOCOL_H

#include "./defines.h"

// protocol functions
int llopen(int port, int flag);
int llwrite(int fd, unsigned char *buffer, int length);
int llread(int fd, unsigned char *packets);
int llclose(int fd, int flag);

// helper functions
unsigned char *bcc2_calc(unsigned char *message, int length);
void alarmHandler();

#endif
