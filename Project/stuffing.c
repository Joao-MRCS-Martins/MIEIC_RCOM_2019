#include "stuffing.h"
#include <stdio.h>
#include <stdlib.h>

void bcc2_stuffing(unsigned char *bcc2, int *size,
                   unsigned char *bcc2_stuffed) {
  if (*bcc2 == FLAG) {
    bcc2_stuffed[0] = ESCAPE;
    bcc2_stuffed[1] = FLAG_ESC;
    *size = 2;
  } else if (*bcc2 == ESCAPE) {
    bcc2_stuffed[0] = ESCAPE;
    bcc2_stuffed[1] = ESC_ESC;
    *size = 2;
  } else {
    *bcc2_stuffed = *bcc2;
    *size = 1;
  }
}

unsigned char *data_stuffing(unsigned char *data, int size, int *final_size) {
  *final_size = size;
  unsigned char *stuffed_data =
      (unsigned char *)malloc(size * sizeof(unsigned char));

  int j = 0; // current position on stuffed array
  for (int i = 0; i < size; i++) {
    if (data[i] == FLAG) {
      stuffed_data = (unsigned char *)realloc(stuffed_data, ++(*final_size));
      stuffed_data[j] = ESCAPE;
      stuffed_data[j + 1] = FLAG_ESC;
      j += 2;
    } else if (data[i] == ESCAPE) {
      stuffed_data = (unsigned char *)realloc(stuffed_data, ++(*final_size));
      stuffed_data[j] = ESCAPE;
      stuffed_data[j + 1] = ESC_ESC;
      j += 2;
    } else {
      stuffed_data[j] = data[i];
      j++;
    }
  }

  return stuffed_data;
}

unsigned char *data_destuffing(unsigned char *data, int size, int *final_size) {
  *final_size = size;
  unsigned char *stuffed_data =
      (unsigned char *)malloc(size * sizeof(unsigned char));

  int sizeless = 0;
  int j = 0;
  for (int i = 0; i < size;) {

    if (i == size - 1) {
      stuffed_data[j] = data[i];
      i++;
    } else {
      if (data[i] == ESCAPE && data[i + 1] == FLAG_ESC) {
        stuffed_data[j] = FLAG;
        i += 2;
        sizeless++;
      } else if (data[i] == ESCAPE && data[i + 1] == ESC_ESC) {
        stuffed_data[j] = ESCAPE;
        i += 2;
        sizeless++;
      } else {
        stuffed_data[j] = data[i];
        i++;
      }
    }
    j++;
  }
  stuffed_data =
      (unsigned char *)realloc(stuffed_data, (*final_size) - sizeless);
  *final_size -= sizeless;
  return stuffed_data;
}

void bcc2_destuffing(unsigned char *bcc2_s, unsigned char *bcc2) {
  if (bcc2_s[0] == ESCAPE) {

    if (bcc2_s[1] == ESC_ESC)
      *bcc2 = ESCAPE;
    else
      *bcc2 = FLAG;

  } else {
    *bcc2 = *bcc2_s;
  }
}
