#ifndef _STUFFING_H
#define _STUFFING_H

#include "defines.h"

unsigned char* bcc2_stuffing(unsigned char bcc2, int *size);
void stuffing(unsigned char* array, int size);

#endif 