#include <stdio.h>
#include <stdlib.h>

#include "./app_layer.h"

int sender_app(int port, char * file) {
  //abrir ficheiro
  //repartir em pacotes
  //preparar pacote de controlo start e end
  //abrir ligacao llopen
  //preparar pacotes de dados
  //enviar pacote de controlo start
  //enviar pacotes de dados com llwrite, esperando pela sua conclusao
  //enviar pacote de controlo end
  //terminar ligacao llclose
  printf("File name: %s\n",file);
  return 0;
}

int receiver_app(int port) {
  //abrir ligacao llopen
  //receber pacote de controlo start llread
  //abrir/criar ficheiro indicado no pacote de controlo start
  //processar e preparar rececao de pacotes de dados
  //receber pacotes de dados e processar correcao llread
  //receber pacote de controlo end e validar
  //terminar ligacao llclose
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
    return sender_app(port,argv[3]);
  }
  else if(actor == RECEIVER) {
    return receiver_app(port);
  }
  else {
    printf("Invalid actor.\n");
    return INVALID_ACTOR;
  }
}