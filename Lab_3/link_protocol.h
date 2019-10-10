#ifndef _LINK_PROTOCOL_H
#define _LINK_PROTOCOL_H

int llopen(int port, int flag);
int llwrite(int fd, char *buffer, int length);
int llread(int fd, char *buffer);
int llclose(int fd);


#endif 