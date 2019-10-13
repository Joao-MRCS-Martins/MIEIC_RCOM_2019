#ifndef _LINK_PROTOCOL_H
#define _LINK_PROTOCOL_H

#include "./defines.h"

// protocol functions
int llopen(int port, int flag);
int llwrite(int fd, char *buffer, int length);
int llread(int fd, char *info);
int llclose(int fd, int flag);

// helper functions
int send_SET();
int send_DISC();
int send_UA();
unsigned char bcc_calc(unsigned char a, unsigned char c);
unsigned char *bcc2_calc(unsigned char *message, int length);
void alarmHandler();

#endif