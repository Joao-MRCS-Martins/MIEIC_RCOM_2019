#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
// ADDED HEADERS
#include "./link_protocol.h"
#include "./state_machine_frame.h"
#include "./stuffing.h"

struct termios oldtio;
struct control_frame message;

int fd;
int n_try = 0;
int alrmSet = FALSE;
int n_seq = 0;

unsigned char *bcc2_calc(char *message, int length) {
  unsigned char *bcc2 = (unsigned char *)malloc(sizeof(unsigned char));
  *bcc2 = message[0];
  for (int i = 1; i < length; i++) {
    *bcc2 ^= message[i];
  }

  return bcc2;
}

void alarmHandler() {
  alrmSet = TRUE;
  n_try++;
  printf("Alarm nº%d\n", n_try);
}

void alarmHandlerR() {
  printf("Timeout. exiting ...\n");
  exit(0);
}

/*this needs to change to a generic one*/
int send_message() { return write(fd, &message, sizeof(struct control_frame)); }

unsigned char bcc_calc(unsigned char a, unsigned char c) { return a ^ c; }

int llopen(int port, int flag) {
  char port_path[MAX_BUFF];
  struct termios newtio;
  struct header_fields header;
  unsigned char aux;
  int state = 0;

  signal(SIGALRM, alarmHandler);

  if (port < 0 || port > 2) {
    return INVALID_PORT;
  }

  if (flag != TRANSMITTER && flag != RECEIVER) {
    return INVALID_ACTOR;
  }

  sprintf(port_path, "/dev/ttyS%d", port);

  fd = open(port_path, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(port_path);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("Serial port set up.\n");

  if (flag == TRANSMITTER) {
    // FILL SET FRAME
    message.flag_i = message.flag_f = FLAG;
    message.a = A_SENDER;
    message.c = C_SET;
    message.bcc = bcc_calc(message.a, message.c);
    header.A_EXCT = A_SENDER;
    header.C_EXCT = C_UA;
	
	printf("Sending SET frame\n");

    do {
      send_message();
      alarm(TIMEOUT);
      alrmSet = FALSE;
		printf("Message sent. Processing UA.\n");
      while (!alrmSet && state != STOP_S) {
        read(fd, &aux, 1);
		printf("Char read: %x\n",aux);
        state_machine(&state, aux, &header);
      }

      if (state == STOP_S)
        break;
    } while (n_try < MAX_RETRIES);
	
	
    if (n_try == MAX_RETRIES)
      return TIMEOUT_ERROR;

	printf("Sender open link successfully.\n");
  }

  if (flag == RECEIVER) {
    header.A_EXCT = A_SENDER;
    header.C_EXCT = C_SET;
    alarm(TIMEOUT);

	printf("Processing SET.\n"); 
    while (state != STOP_S) {
      if (alrmSet) {
        return TIMEOUT_ERROR;
      }

      read(fd, &aux, 1);
      state_machine(&state, aux, &header);
    }
    // FILL UA FRAME
    message.flag_i = message.flag_f = FLAG;
    message.a = A_SENDER;
    message.c = C_UA;
    message.bcc = bcc_calc(message.a, message.c);
	printf("Sending UA frame.\b");
    write(fd, &message, sizeof(struct control_frame));
	printf("Receiver open link successfully.\n");
  }

  return fd;
}

int llwrite(int fd, char *buffer, int length) {

  struct info_frame message;
  struct header_fields header;
  unsigned char aux;
  int state = 0;

  message.flag_i = message.flag_f = FLAG;
  message.a = A_SENDER;
  if (n_seq == 0) // check sequence number
    message.c = C_S0;
  else
    message.c = C_S1;
  message.bcc1 = message.a ^ message.c;

  // bcc2 generation & stuffing
  unsigned char *bcc2 = bcc2_calc(buffer, length);
  unsigned char *bcc2_stuffed = bcc2_stuffing(bcc2);
  message.bcc2 = bcc2_stuffed;

  // byte stuffing on file data
  message.data = data_stuffing(buffer, length, &message.data_size);

  // prepare reply processing
  header.A_EXCT = A_SENDER;
  header.C_EXCT = (n_seq == 0) ? RR_R0 : RR_R1;
  n_try = 0;

  do {

    write(fd, &message, sizeof(struct info_frame));
    alrmSet = FALSE;
    alarm(TIMEOUT);

    while (!alrmSet && state != STOP_S) {
      read(fd, &aux, 1);
      state_machine(&state, aux, &header);
      if ((aux == REJ_R0 && n_seq == 0) || (aux == REJ_R1 && n_seq == 1))
        break;
    }

    if (state == STOP_S)
      break;
  } while (n_try < MAX_RETRIES);

  if (n_try == MAX_RETRIES)
    return TIMEOUT_ERROR;

  n_seq ^= 1; // PLACE WHERE RR IS CORRECTLY RECEIVED

  return length;
}

volatile int STOP = FALSE;

int llread(int fd, unsigned char *packets) {

  unsigned char buffer;
  unsigned char *bcc_data = (unsigned char *)malloc(sizeof(unsigned char *));
  int i = 0;
  int state = 0;
  int flag_answer = 0;

  struct control_frame message;

  signal(SIGALRM, alarmHandlerR);

  while (state == STOP_I) {
    alarm(TIMEOUT_R);
    read(fd, &buffer, 1);
    state_machine_I(&state, buffer, bcc_data, packets, flag_answer);
    i++;
  }

  unsigned *final_size = (unsigned *)malloc(sizeof(unsigned *));
  
  bcc2_destuffing(bcc_data);
  data_destuffing(packets, sizeof(packets), final_size);

  if (bcc_data == bcc2_calc(packets, strlen((const char *)packets))) {
    if (flag_answer == C_S0)
      message.c = RR_R0;
    else
      message.c = RR_R1;
  } else {
    if (flag_answer == C_S0)
      message.c = REJ_R0;
    else
      message.c = REJ_R1;
  }

  message.flag_i = message.flag_f = FLAG;
  message.a = A_SENDER;
  message.c = flag_answer;
  message.bcc = bcc_calc(message.a, message.c);

  write(fd, &message, sizeof(struct control_frame));

  return 0;
}

int llclose(int fd, int flag) {
  int state = 0;
  unsigned char buffer;

  if (flag == TRANSMITTER) {
    struct header_fields fields;

    fields.A_EXCT = A_RECEIVER;
    fields.C_EXCT = C_DISC;

    signal(SIGALRM, alarmHandler);

    message.a = A_SENDER;
    message.c = C_DISC;
    message.bcc = bcc_calc(message.a, message.c);
    message.flag_i = message.flag_f = FLAG;

	printf("Sending DISC to RECEIVER.\n");

    do {
	  send_message();
      printf("DISC sent.\n");
      alarm(TIMEOUT);
      alrmSet = FALSE;
      while (!alrmSet && state != STOP_S) {
        alarm(TIMEOUT);
        read(fd, &buffer, 1);
		printf("Char read: %x\n",buffer);
        state_machine(&state, buffer, &fields);
		printf("State: %d\n",state);
      }

      if (state == STOP_S)
        break;
    } while (n_try < MAX_RETRIES);

    if (n_try == MAX_RETRIES)
      return TIMEOUT_ERROR;

	printf("DISC received. Sending UA to RECEIVER.\n");

    message.a = A_RECEIVER;
    message.c = C_UA;
    message.bcc = bcc_calc(message.a, message.c);
    message.flag_i = message.flag_f = FLAG;

    do {
      if (send_message() > 0)
		break;
      alarm(TIMEOUT);
    } while (n_try < MAX_RETRIES);

    if (n_try == MAX_RETRIES)
      return TIMEOUT_ERROR;
  }

  if (flag == RECEIVER) {
    struct header_fields fields;

    fields.A_EXCT = A_SENDER;
    fields.C_EXCT = C_DISC; // can change

    signal(SIGALRM, alarmHandlerR);
	
	printf("Reading DISC\n.");

    while (state != STOP_S) {
      alarm(TIMEOUT_R);
      read(fd, &buffer, 1);
      state_machine(&state, buffer, &fields);
    }
	
	printf("Sending DISC to TRANSMITTER.\n");

    message.a = A_RECEIVER;
    message.c = C_DISC;
    message.bcc = bcc_calc(message.a, message.c);
    message.flag_i = message.flag_f = FLAG;
	fields.A_EXCT = A_RECEIVER;
	fields.C_EXCT = C_UA;
    write(fd, &message, sizeof(struct control_frame));
    alarm(TIMEOUT_R);
	
	printf("Waiting for UA and processing.\n");
	
	state = 0;
    while (state != STOP_S) {
      alarm(TIMEOUT_R);
      read(fd, &buffer, 1);
      state_machine(&state, buffer, &fields);
    }
  }
	printf("Successfully closed link.\n");
  sleep(1);
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);
  return 0;
}

int main(int argc, char* argv[]) {
    printf("Usage: link [port](0|1|2) [transmitter|receiver](0,1) \n");
    if(argc < 3) {
      printf("Wrong arguments.\n");
      return -5;
    }

	int fd = llopen(atoi(argv[1]),atoi(argv[2]));

	if (atoi(argv[2]) == 0)  {
		char mensagem[10] = "cenasmenas";	
		int n = llwrite(fd,mensagem,10);
		printf("n: %d\n",n);
	}
	else {
		unsigned char mensagem[255];
		int n = llread(fd,mensagem);
		printf("n: %d\n",n);
	}
    return llclose(fd,atoi(argv[2]));

    // char cenas[5] = {0x45, 0x7D, 0x5D, 0x67, 0x34};
    // int final;
    // unsigned char *data_destuffed = data_destuffing(cenas, 5, &final);
    // printf("Stuffed array:\n");
    // for (int i = 0; i < final; i++) {
    //   printf("data_stuffed[%d]: %x, %d\n", i, data_destuffed[i], final);
    // }
    // unsigned char *bcc2 = (unsigned char*) malloc(sizeof(unsigned char));
    // *bcc2 = FLAG;
    // unsigned char * stuffed = bcc2_stuffing(bcc2);
    // printf("BCC stuffed: %x%x\n",stuffed[0],stuffed[1]);

		// char cenas [5] = {0x45,0x7E,0x12,0x7D,0x7E};
		// int final;
		// unsigned char *data_stuffed = data_stuffing(cenas,5,&final);
		// printf("Stuffed array:\n");
		// for(int i = 0; i < final; i++) {
		// 	printf("data_stuffed[%d]: %x\n",i,data_stuffed[i]);
		// }

		// free(bcc2);
		// free(stuffed);
		// free(data_stuffed);

}
