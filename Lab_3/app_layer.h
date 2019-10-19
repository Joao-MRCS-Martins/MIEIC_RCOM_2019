#ifndef _APP_LAYER_H
#define _APP_LAYER_H

#include "./defines.h"

char *getFileData(char *filename, int *file_size);
unsigned char *makeControlPacket(char *filename, int size, int * packet_size);
unsigned char *makeDataPacket(char* data, int *index, int *packet_size, int data_size);
int senderApp(int port, char *file);

unsigned char *getStartInfo(int fd, char *filename, int *file_size);
int getPacketInfo(int port_fd,int dest_fd, int *total_read);
int checkEndInfo(int fd, unsigned char *control_packet);
int receiverApp(int port);
#endif 