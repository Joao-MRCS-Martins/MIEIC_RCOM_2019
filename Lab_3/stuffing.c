  #define FLAG 0x7e
  #define ESCAPE 0x7d

  #include <stdio.h>

  void stuffing(unsigned char* array, int size){
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

  int main(int argc, char** argv){
    //if(argc < 2)
    //  exit(1);

    unsigned char ar[2] = {FLAG,ESCAPE};
    stuffing(ar, 2);

    return 0;
  }
