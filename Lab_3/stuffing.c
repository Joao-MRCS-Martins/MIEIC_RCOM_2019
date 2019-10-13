#include <stdio.h>
#include <stdlib.h>
#include "stuffing.h"


unsigned char *bcc2_stuffing(unsigned char *bcc2) {
    unsigned char* bcc2_stuffed;
    if(bcc2 == FLAG) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
        sprintf(bcc2_stuffed,"%x",FLAG_ESC);
    }
    else if(bcc2 == ESCAPE) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
       sprintf(bcc2_stuffed,"%x",ESC_ESC);
    }
    else {
        bcc2_stuffed = bcc2;
    }

    return bcc2_stuffed;
}

unsigned char *data_stuffing(unsigned char* data, int size){
  int final_size = size;
  for(; *array != '\0'; array++){
    if(*array == ESCAPE || *array == FLAG)
      final_size++;
  }
  printf("%d\n", final_size);
  unsigned char ret_vect[final_size];
  printf("%lu\n", sizeof(ret_vect));
  unsigned char * p = ret_vect;
  for(; *p != '\0'; p++){
    if(*p == ESCAPE){
      p++;
      *p = ESCAPE ^ 0x20;
    }
    else if(*p == FLAG){
      *p = ESCAPE;
      p++;
      *p = FLAG ^ 0x20;
    }
  }
  
  for(long unsigned int i = 0; i < sizeof(ret_vect); i++){
    printf("%u\n", ret_vect[i]);
  }
}
