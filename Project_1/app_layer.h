#ifndef _APP_LAYER_H
#define _APP_LAYER_H

#include "./defines.h"

char *getFileData(char *filename, int *file_size);
void makeControlPacket(char *filename, int size, int *packet_size,
                       unsigned char *c_packet);
int makeDataPacket(char *data, int *index, unsigned char *packet,
                   int *packet_size, int data_size);
int senderApp(int port, char *file);

int getStartInfo(int fd, char *filename, int *file_size,
                 unsigned char *c_packet);
int getPacketInfo(int port_fd, int dest_fd, int *total_read);
int checkEndInfo(int fd, unsigned char *control_packet);
int receiverApp(int port);
#endif