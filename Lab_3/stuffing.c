#include "stuffing.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char* bcc2_stuffing(unsigned char* bcc2)
{
    unsigned char* bcc2_stuffed;
    if(*bcc2 == FLAG) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
        bcc2_stuffed[0] = ESCAPE;
        bcc2_stuffed[1] = FLAG_ESC;
    }
    else if(*bcc2 == ESCAPE) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
        bcc2_stuffed[0] = ESCAPE;
        bcc2_stuffed[1] = ESC_ESC;
    }
    else {
        bcc2_stuffed = bcc2;
    }

    return bcc2_stuffed;
}

unsigned char *data_stuffing(char* data, int size, unsigned *final_size) {
  *final_size = size;
  unsigned char *stuffed_data = (unsigned char *) malloc(size * sizeof(unsigned char));
  
  int j = 0;//current position on stuffed array
  for(int i = 0; i < size; i++){
    if(data[i] == ESCAPE) {
      stuffed_data = (unsigned char *) realloc(stuffed_data,++(*final_size));
      stuffed_data[j] = ESCAPE;
      stuffed_data[j+1] = ESC_ESC;
      j +=2;
    }

    return stuffed_data;
}
