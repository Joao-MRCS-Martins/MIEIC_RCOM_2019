#ifndef _APP_LAYER_H
#define _APP_LAYER_H

#include "./defines.h"

char *getFileData(char *filename, int *file_size);
int senderApp(int port, char *file);
int receiverApp(int port);
#endif