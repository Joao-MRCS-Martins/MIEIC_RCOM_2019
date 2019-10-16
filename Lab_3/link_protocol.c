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

int fd;
int n_try = 0;
int alrmSet = FALSE;
int n_seq = 0;

unsigned char *bcc2_calc(unsigned char *message, int length) {
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

unsigned char bcc_calc(unsigned char a, unsigned char c) { return a ^ c; }

int llopen(int port, int flag) {

  unsigned char frame[256];
  char port_path[MAX_BUFF];
  struct termios newtio;
  struct header_fields header;
  unsigned char aux;
  int state = 0;

  signal(SIGALRM, alarmHandler);

  /*if (port < 0 || port > 2) {
    return INVALID_PORT;
  }*/

  if (flag != TRANSMITTER && flag != RECEIVER) {
    return INVALID_ACTOR;
  }

  // sprintf(port_path, "/dev/ttyS%d", port);
  sprintf(port_path, "/dev/pts/%d", port);

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
  newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("Serial port set up.\n");

  if (flag == TRANSMITTER) {
    // FILL SET FRAME
    frame[0] = frame[4] = FLAG;
    frame[1] = A_SENDER;
    frame[2] = C_SET;
    frame[3] = bcc_calc(frame[1], frame[2]);
    header.A_EXCT = A_SENDER;
    header.C_EXCT = C_UA;

    printf("Sending SET frame\n");

    do {
      write(fd, &frame,5);
      alarm(TIMEOUT);
      alrmSet = FALSE;
      printf("Message sent. Processing UA.\n");
      while (!alrmSet && state != STOP_S) {
        read(fd, &aux, 1);
        state_machine(&state, aux, &header);
      }

      if (state == STOP_S) {
        break;
      }
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
    frame[0] = frame[4] = FLAG;
    frame[1] = A_SENDER;
    frame[2] = C_UA;
    frame[3] = bcc_calc(frame[1], frame[2]);
    printf("Sending UA frame.\n");
    write(fd, &frame, 5);
    printf("Receiver open link successfully.\n");
  }

  return fd;
}

int llwrite(int fd, unsigned char *buffer, int length) {

  unsigned char frame[256];
  struct header_fields header;
  unsigned char aux;
  int state = 0;

  frame[0] = FLAG;
  frame[1] = A_SENDER;
  if (n_seq == 0) // check sequence number
    frame[2] = C_S0;
  else
    frame[2] = C_S1;
  frame[3] = frame[1] ^ frame[2];

  // byte stuffing on file data
  int datasize = 0;
  unsigned char *data = data_stuffing(buffer, length, &datasize);
  strncpy(&frame[4], data, datasize);

  // bcc2 generation & stuffing
  int bccsize = 0;
  unsigned char *bcc2 = bcc2_calc(buffer, length);
  unsigned char *bcc2_stuffed = bcc2_stuffing(bcc2, &bccsize);
  strncpy(&frame[4 + datasize], bcc2_stuffed, bccsize);
  frame[4+datasize + bccsize] = FLAG;
  // prepare reply processing
  header.A_EXCT = A_SENDER;
  header.C_EXCT = (n_seq == 0) ? RR_R0 : RR_R1;
  n_try = 0;

  do {

    write(fd, &frame, datasize + bccsize + 5);
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

  printf("Written successfully.\n");
  return length;
}

int llread(int fd, unsigned char *packets) {

  unsigned char frame[256];
  // state machine
  int state = 0;
  int REJ1 = 0;
  int REJ0 = 0;
  // read information auxiliares
  unsigned char buffer;
  int state_read = 0;
  int flag_answer = 0;

  unsigned char *bcc_data =  (unsigned char *)malloc(2 * sizeof(unsigned char));;

  signal(SIGALRM, alarmHandlerR);
  int datasize = 0;
  while(state != END_R )
  {
  switch (state) {
  case READ_R:
    while (state_read != STOP_I) {
      alarm(TIMEOUT_R);
      read(fd, &buffer, 1);
      printf("read char %x\n", buffer);
      state_machine_I(&state_read, buffer, packets, bcc_data, flag_answer, &datasize);
    }
    state = ANALIZE_R;

    break;
  case ANALIZE_R:
  printf("\n");
    unsigned char *bcc2 = bcc2_destuffing(bcc_data);
    int final_size;
    unsigned char *dest_data = data_destuffing(packets, datasize, &final_size);
    unsigned char *packets_bcc = bcc2_calc(dest_data,final_size);
    
    if (*bcc2 == *packets_bcc) {
      if (flag_answer == C_S0)
      {
        frame[2] = RR_R0;
      }
      else
        frame[2] = RR_R1;

    } else {
      if (flag_answer == C_S0)
        frame[2] = REJ_R0;
      else
        frame[2] = REJ_R1;
    }

    frame[0] = FLAG;
    frame[1] = A_SENDER;
    frame[3] = bcc_calc(frame[1], frame[2]);
    frame[4] = FLAG;
    state = WRITE_R;
    break;
  case WRITE_R:
    printf("meias\n");
    printf("c in llread %x", frame[2]);
    write(fd, &frame, 5);
    if (frame[2] == REJ_R0) {
      if (REJ0 < MAX_REJ) {
        REJ0++;
        state = READ_R;
      } else {
        REJ0 = 0;
        alarm(TIMEOUT_R);
      }
    } else if (frame[2] == REJ_R1) {
      if (REJ1 < MAX_REJ) {
        REJ1++;
        state = READ_R;
      } else {
        REJ1 = 0;
        alarm(TIMEOUT_R);
      }
    } else
      state = END_R;
  case END_R:
    break;
  }
  }
  return strlen(packets);
}

int llclose(int fd, int flag) {
  int state = 0;
  unsigned char buffer;
  unsigned char frame[256];

  if (flag == TRANSMITTER) {
    struct header_fields fields;

    fields.A_EXCT = A_RECEIVER;
    fields.C_EXCT = C_DISC;

    signal(SIGALRM, alarmHandler);

    frame[0] = frame[4] = FLAG;
    frame[1] = A_SENDER;
    frame[2] = C_DISC;
    frame[3] = bcc_calc(frame[1], frame[2]);

    printf("Sending DISC to RECEIVER.\n");

    do {
       write(fd, &frame,5);
      printf("DISC sent.\n");
      alarm(TIMEOUT);
      alrmSet = FALSE;
      while (!alrmSet && state != STOP_S) {
        alarm(TIMEOUT);
        read(fd, &buffer, 1);
        printf("Char read: %x\n", buffer);
        state_machine(&state, buffer, &fields);
        printf("State: %d\n", state);
      }

      if (state == STOP_S)
        break;
    } while (n_try < MAX_RETRIES);

    if (n_try == MAX_RETRIES)
      return TIMEOUT_ERROR;

    printf("DISC received. Sending UA to RECEIVER.\n");

    frame[1] = A_RECEIVER;
    frame[2] = C_UA;
    frame[3] = bcc_calc(frame[1], frame[2]);
    frame[0] = frame[4] = FLAG;

    do {
      if (  write(fd, &frame, 5) > 0)
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

    frame[1] = A_RECEIVER;
    frame[2] = C_DISC;
    frame[3] = bcc_calc(frame[1], frame[2]);
    frame[0] = frame[4] = FLAG;
    fields.A_EXCT = A_RECEIVER;
    fields.C_EXCT = C_UA;
    write(fd, &frame, 5);
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

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Wrong arguments.\n");
    return -5;
  }

  int fd = llopen(atoi(argv[1]), atoi(argv[2]));

  if (atoi(argv[2]) == 0) {
    unsigned char mensagem[] = {FLAG, 0x43, 0x12, ESCAPE, FLAG};
    int n = llwrite(fd, mensagem, 4);
  } else {
    unsigned char mensage[255];
    int n = llread(fd, mensage);
    printf("n: %d\n", n);
  }
  return llclose(fd, atoi(argv[2]));

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
