#ifndef _LINK_PROTOCOL_H
#define _LINK_PROTOCOL_H

#include "defines.h"

int llopen(int port, int flag);
int llwrite(int fd, char *buffer, int length);
int llread(int fd, struct Message *message);
int llclose(int fd);


#endif 