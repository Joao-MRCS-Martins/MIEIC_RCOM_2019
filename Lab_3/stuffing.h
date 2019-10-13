#ifndef _STUFFING_H
#define _STUFFING_H

#include "defines.h"

unsigned char* bcc2_stuffing(unsigned char *bcc2);
unsigned char *data_stuffing(char* data, int size, int *final_size);

#endif