#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "./app_layer.h"
#include "./link_protocol.h"

char *getFileData(char *filename, int *file_size) {
  FILE *f;
  struct stat meta;
  
  if((f = fopen(filename,"r")) == NULL) {
    perror(filename);
    return NULL;
  }

  if(stat(filename,&meta) <0 ) {
    perror(filename);
    return NULL;
  }

  *file_size = meta.st_size;
  char *file_data = (char *) malloc(*file_size);

  fread(file_data,sizeof(char),*file_size,f);
  fclose(f);
  return file_data;
}

unsigned char *getControlPacket(char *filename, int size) {
  unsigned char * packet = (unsigned char *) malloc(9 * sizeof(unsigned char) + strlen(filename));
  packet[0] = C_START;
  packet[1] = T_SIZE;
  packet[2] = L1_S;
  packet[3] = (size >> 24) & 0xFF;
  packet[4] = (size >> 16) & 0xFF;
  packet[5] = (size >> 8) & 0xFF;
  packet[6] = size & 0xFF;
  packet[7] = T_NAME;
  packet[8] = strlen(filename);
  for(int i = 0; i < packet[8]; i++) {
    packet[9+i] = filename[i];
  }

  return packet;
}

unsigned char *getDataPacket(char* data, int *index, int *packet_size, int data_size) {
  return NULL;
}

int senderApp(int port, char * file) {  
  int data_size;
  char * data = getFileData(file, &data_size);
  
  unsigned char * c_packet = getControlPacket(file,data_size);
  printf("c_packet: %s\n", &c_packet[9]);
  
  int fd = llopen(port,TRANSMITTER);
  printf("fd: %d\n",fd);
  
  //enviar pacote de controlo start
  if(llwrite(fd,(char *) c_packet,data_size) < 0) {
    printf("Failed to send start packet.\n");
    return STRT_PCKT;
  }

  int index = 0;
  int packet_size;
  unsigned char *d_packet;
  while(1) {
    if((d_packet = getDataPacket(data,&index,&packet_size,data_size)) == NULL) {
      break;
    }

    if(llwrite(fd,(char *) d_packet, packet_size) < 0) {
      printf("Failed to send data packet.\n");
      free(c_packet);
      free(data);
      free(d_packet);
      return DATA_PCKT;
    }

    free(d_packet);
  }
  
  //enviar pacote de controlo end
  c_packet[0] = C_END;
  if(llwrite(fd,(char *) c_packet,data_size) < 0) {
    printf("Failed to send end packet.\n");
    return END_PCKT;
  }
  //terminar ligacao llclose
  free(c_packet);
  free(data);
  free(d_packet);
  llclose(fd,TRANSMITTER);
  return 0;
}

int receiverApp(int port) {
  //abrir ligacao llopen
  //receber pacote de controlo start llread
  //abrir/criar ficheiro indicado no pacote de controlo start
  //processar e preparar rececao de pacotes de dados
  //receber pacotes de dados e processar correcao llread
  //receber pacote de controlo end e validar
  //terminar ligacao llclose
  printf("port: %d\n",port);
  return 0;
}

int main(int argc, char *argv[]) {

  if(argc != 4) {
    printf("Wrong number of arguments. Usage: ./link <TRANSMITTER|RECEIVER>[0|1] <SERIAL PORT>[0|1|2] <file>\n");
    return INVALID_ARGS;
  }

  int port = atoi(argv[2]);
  if(port < 0 || port > 2) {
    printf("Wrong Port. Use ports 0, 1 or 2.\n");
    return INVALID_PORT;
  }

  int actor = atoi(argv[1]);
  if(actor == TRANSMITTER) {
    return senderApp(port,argv[3]);
  }
  else if(actor == RECEIVER) {
    return receiverApp(port);
  }
  else {
    printf("Invalid actor.\n");
    return INVALID_ACTOR;
  }
}