#ifndef _STUFFING_H
#define _STUFFING_H

#include "defines.h"

void bcc2_stuffing(unsigned char bcc2, unsigned char *bcc2_stuffed);
void data_destuffing(unsigned char *data, int size, unsigned *final_size, unsigned char *stuffed_data);
void data_stuffing(unsigned char *data, int size, int *final_size, unsigned char * stuffed_data);
unsigned char *bcc2_destuffing(unsigned char *bcc2);

#endif
