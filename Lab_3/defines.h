#ifndef _DEFINES_H_
#define _DEFINES_H_

///////////////// LINK LAYER /////////////////////////////
// HARDWARE
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define MODEMDEVICE "/dev/ttyS1"

// BOOLEAN
#define FALSE 0
#define TRUE 1

// FRAME
#define FLAG 0x7E

// ADDRESS FIELD
#define A_RECEIVER 0x01
#define A_SENDER 0x03

// CONTROL FIELD
#define C_SET 0x03
#define C_UA 0x07
#define C_S0 0x00
#define C_S1 0x40
#define RR_R0 0x05
#define RR_R1 0x85
#define REJ_R0 0x01
#define REJ_R1 0x81
#define C_DISC 0x0B

// ESCAPE OCTETS
#define ESCAPE 0x7d
#define FLAG_ESC 0x5E
#define ESC_ESC 0x5D

// STATE MACHINE
#define START_S 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define STOP_S 5

// STATE MACHINE FRAMEI
#define INFO 5
#define STOP_I 6

// STATE MACHINE LLREAD
#define READ_R 0
#define ANALIZE_R 1
#define WRITE_R 2
#define END_R 3

// EXECUTION SPECS
#define MAX_RETRIES 3
#define TIMEOUT 3
#define TIMEOUT_R 10
#define MAX_BUFF 255
#define MAX_REJ 3

// SERIAL PORTS
#define ttyS0 0
#define ttyS1 1
#define ttyS2 2

// ACTOR MODE
#define TRANSMITTER 0
#define RECEIVER 1

// ERROR MESSAGES
#define INVALID_PORT -1
#define INVALID_ACTOR -2
#define INVALID_ARGS -3
#define TIMEOUT_ERROR -4

struct control_frame {
  unsigned char flag_i;
  unsigned char a;
  unsigned char c;
  unsigned char bcc;
  unsigned char flag_f;
};

struct info_frame {
  unsigned char flag_i;
  unsigned char a;
  unsigned char c;
  unsigned char bcc1;
  char *data;
  int data_size;
  unsigned char *bcc2;
  unsigned char flag_f;
};

///// APPLICATION LAYER /////                                                                                                 
#define MAX_SIZE 4096 

struct data_packet {                                                                                                  
  unsigned char C ;                                                                                                 
  unsigned char N;                                                                                                  
  unsigned char L2;                                                                                                 
  unsigned char L1;                                                                                                 
  char * data;
};

struct control_packet {
  unsigned char C;
  struct tlv **tlvs;
};

struct tlv {
  unsigned char T;
  unsigned char L;
  unsigned char V;
};

struct link_layer {
  int port; /*Dispositivo /dev/ttySx, x = 0, 1*/
  int baudRate; /*Velocidade de transmissão*/
  unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
  unsigned int timeout; /*Valor do temporizador: 1 s*/
  unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
  char frame[MAX_SIZE];
 /*Trama*/

};

#endif //_DEFINES_H_
