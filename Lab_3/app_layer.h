#ifndef _APP_LAYER_H
#define _APP_LAYER_H

#include "./defines.h"

char *getFileData(char *filename, int *file_size);
unsigned char *getControlPacket(char *filename, int size);
unsigned char *getDataPacket(char* data, int *index, int *packet_size, int data_size);
int senderApp(int port, char *file);
int receiverApp(int port);
#endif