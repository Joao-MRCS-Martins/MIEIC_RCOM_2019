#include "stuffing.h"
#include <stdio.h>
#include <stdlib.h>

void bcc2_stuffing(unsigned char bcc2, unsigned char *bcc2_stuffed){
  if (bcc2 == FLAG) {
    bcc2_stuffed[0] = ESCAPE;
    bcc2_stuffed[1] = FLAG_ESC;
  } else if (bcc2 == ESCAPE) {
    bcc2_stuffed[0] = ESCAPE;
    bcc2_stuffed[1] = ESC_ESC;
    *size = 2;
  } else {
    bcc2_stuffed[0] = bcc2;
  }
}

void data_stuffing(unsigned char *data, int size, int *final_size, unsigned char * stuffed_data) {
  *final_size = 0;

  int j = 0; // current position on stuffed array
  for (int i = 0; i < size; i++) {
    if (data[i] == FLAG) {
      stuffed_data[j] = ESCAPE;
      stuffed_data[j + 1] = FLAG_ESC;
      j += 2;
	*final_size = *final_size +2;
    } else if (data[i] == ESCAPE) {
      stuffed_data[j] = ESCAPE;
      stuffed_data[j + 1] = ESC_ESC;
      j += 2;
*final_size = *final_size +2;
    } else {
      stuffed_data[j] = data[i];
      j++;
*final_size = *final_size +1;
    }
  }

stuffed_data[j] = '\0';
*final_size = *final_size +1;

}

void data_destuffing(unsigned char *data, int size, unsigned *final_size, unsigned char *stuffed_data) {
  *final_size = size;


  int j = 0;

  for (int i = 0; i < size;) {
    if (data[i] == ESCAPE) {
      if (data[i + 1] == FLAG_ESC)
        stuffed_data[j] = FLAG;
      else
        stuffed_data[j] = ESCAPE;
      i += 2;
	*final_size = *final_size -1;
    } else {
      stuffed_data[j] = data[i];
      i++;

    }
    j++;
  }
  stuffed_data =
      (unsigned char *)realloc(stuffed_data, (*final_size) - sizeless);


}

unsigned char *bcc2_destuffing(unsigned char *bcc2) {
  unsigned char *bcc2_stuffed;
  if (bcc2[0] == ESCAPE) {

    bcc2_stuffed = (unsigned char *)malloc(sizeof(unsigned char *));
    if (bcc2[1] == ESC_ESC)
      bcc2_stuffed[0] = ESCAPE;
    else
      bcc2_stuffed[0] = FLAG;

  } else {
    bcc2_stuffed = bcc2;
  }
  return bcc2_stuffed;
}
