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

void getControlPacket(char *filename, int size, struct control_packet * packet) {
  packet->C = C_START;
  packet->T1 = T_SIZE;
  packet->L1 = sizeof(int);
  sprintf((char *) packet->V1,"%d",size);
  packet->T2 = T_NAME;
  packet->L2 = strlen(filename);
  strcpy((char *) packet->V2,filename);
}

int senderApp(int port, char * file) {
  struct control_packet c_packet;
  
  int data_size;
  char * data = getFileData(file, &data_size);


  for(int i = 0; i<data_size;i++) {
    printf("%c\n",data[i]);
  }
  
  getControlPacket(file,data_size,&c_packet);
  // printf("Packet C:%x\nPacket T1: %x\nPacket L1:%x\nPacket V1: %s\nPacket T2: %x\nPacket L2:%x\nPacket V2: %s\n",
  //         c_packet.C,c_packet.T1,c_packet.L1,c_packet.V1,c_packet.T2,c_packet.L2,c_packet.V2);
  
  int fd = llopen(port,TRANSMITTER);
  printf("fd: %d\n",fd);
  
  //enviar pacote de controlo start
  
  //enviar pacotes de dados com llwrite, esperando pela sua conclusao
  
  //enviar pacote de controlo end
  
  //terminar ligacao llclose
  free(data);
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