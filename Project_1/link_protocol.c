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
int frame_size = 0;
unsigned char frame[2 * MAX_PCKT_SIZE + 6];
int n_retrans = 0;
int n_dups = 0;

int error_or_not(int probability){
  time_t t;
  srand((unsigned) time(&t));
  if((rand()%100) <= probability) return TRUE;
  else return FALSE;
}

void bcc2_calc(unsigned char *message, int length, unsigned char *bcc2) {
  *bcc2 = message[0];
  for (int i = 1; i < length; i++) {
    *bcc2 ^= message[i];
  }

  if(error_or_not(ERROR_RATIO) == TRUE) *bcc2 ^= message[0]; 
}

void alarmHandler() {
  alrmSet = TRUE;
  n_try++;
  if (n_try == MAX_RETRIES) {
    printf("Timeout. Exiting ...\n");
    exit(TIMEOUT_ERROR);
  } else {
    write(fd, &frame, frame_size);
    alarm(TIMEOUT);
  }
  printf("Alarm nº%d\n", n_try);
}

void alarmHandlerR() {
  printf("Timeout. exiting ...\n");
  exit(TIMEOUT_ERROR);
}

unsigned char bcc_calc(unsigned char a, unsigned char c) { 
  if(error_or_not(ERROR_RATIO) == FALSE) 
    return a ^ c; 
  else 
    return a ^ c ^ a;
  }

int llopen(int port, int flag) {
  char port_path[MAX_BUFF];
  struct termios newtio;
  struct header_fields header;
  unsigned char aux;
  int state = 0;

  if (flag != TRANSMITTER && flag != RECEIVER) {
    return INVALID_ACTOR;
  }

  // sprintf(port_path, "/dev/ttyS%d", port);
  sprintf(port_path, "/dev/ttyS%d", port);

  fd = open(port_path, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(port_path);
    return SETUP_ERROR;
  }

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    return SETUP_ERROR;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0;
  newtio.c_cc[VMIN] = 1;

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    return SETUP_ERROR;
  }

  printf("Serial port set up.\n");

  if (flag == TRANSMITTER) {
    signal(SIGALRM, alarmHandler);
    // FILL SET FRAME
    frame[0] = frame[4] = FLAG;
    frame[1] = A_SENDER;
    frame[2] = C_SET;
    frame[3] = bcc_calc(frame[1], frame[2]);
    frame_size = 5;
    header.A_EXCT = A_SENDER;
    header.C_EXCT = C_UA;

    printf("Sending SET frame\n");

    do {
      write(fd, &frame, frame_size);
      alarm(TIMEOUT);
      printf("Message sent. Processing UA.\n");
      while (state != STOP_S) {
        read(fd, &aux, 1);
        state_machine(&state, aux, &header);
      }

      if (state == STOP_S) {
        break;
      }
    } while (1);

    if (n_try == MAX_RETRIES)
      return TIMEOUT_ERROR;

    printf("Sender open link successfully.\n");
  }

  if (flag == RECEIVER) {
    signal(SIGALRM, alarmHandlerR);

    header.A_EXCT = A_SENDER;
    header.C_EXCT = C_SET;
    alarm(TIMEOUT_R);

    printf("Processing SET.\n");
    while (state != STOP_S) {
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
  memcpy(&frame[4], data, datasize);

  // bcc2 generation & stuffing
  int bccsize = 0;
  unsigned char bcc2;
  bcc2_calc(buffer, length, &bcc2);
  unsigned char bcc2_stuffed[2];
  bcc2_stuffing(&bcc2, &bccsize, bcc2_stuffed);
  memcpy(&frame[4 + datasize], bcc2_stuffed, bccsize);
  frame[4 + datasize + bccsize] = FLAG;
  // prepare reply processing
  header.A_EXCT = A_SENDER;
  header.C_EXCT = (n_seq == 0) ? RR_R1 : RR_R0;
  n_try = 0;
  frame_size = datasize + bccsize + 5;
  do {

    write(fd, &frame, frame_size);
    alrmSet = FALSE;
    alarm(TIMEOUT);
    while (alrmSet != TRUE && state != STOP_S) {
      read(fd, &aux, 1);
      state_machine(&state, aux, &header);
      if ((aux == REJ_R0 && n_seq == 0) || (aux == REJ_R1 && n_seq == 1)) {
        state = 0;
        break;
      }
    }
    if (state == STOP_S)
      break;
  } while (n_try < MAX_RETRIES);

  if (n_try == MAX_RETRIES) {
    free(data);
    return TIMEOUT_ERROR;
  }

  n_seq ^= 1;
  free(data);
  return length;
}

int llread(int fd, unsigned char *packets) {

  // state machine
  int state = 0;
  int dup = 0;
  // read information auxiliares
  unsigned char buffer;
  int state_read = 0;
  int flag_answer = 0;

  unsigned char bcc_data[2];

  signal(SIGALRM, alarmHandlerR);
  int datasize = 0;
  n_try = 0;

  while (state != END_R && n_try < MAX_RETRIES) {
	switch (state) {
    case READ_R:
      do {
        alarm(TIMEOUT_R);
        read(fd, &buffer, 1);
        state_machine_I(&state_read, buffer, packets, bcc_data, &flag_answer,&datasize);
      } while (state_read != STOP_I);
      state = ANALIZE_R;
      break;
    case ANALIZE_R: {
      unsigned char bcc2;
      bcc2_destuffing(bcc_data, &bcc2);
      int final_size;
      unsigned char *dest_data =
          data_destuffing(packets, datasize, &final_size);
      unsigned char packets_bcc;
      bcc2_calc(dest_data, final_size, &packets_bcc);
      if (bcc2 == packets_bcc) {
        if (flag_answer == C_S0)
         {
             frame[2] = RR_R1;
             if(n_seq == 1)
                dup =TRUE;
             else
                memcpy(packets, dest_data, final_size);
        }
           if (flag_answer == C_S1)
           {
             frame[2] = RR_R0;
             if(n_seq == 0)
                dup =TRUE;
             else
                memcpy(packets, dest_data, final_size);
           }
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
      free(dest_data);
      break;
    }

    case WRITE_R:
      write(fd, &frame, 5);
      if (frame[2] == REJ_R0 || frame[2] == REJ_R1 || dup ==TRUE) {
          if(dup) n_dups++;
          else
            n_retrans++;
          printf(" Rejected/duplicate frame\n");
		      state_read = START_S;
          state = READ_R;
      } else {
        state = END_R;
	  	  n_seq ^= 1;
      }
      dup = FALSE;
	  break;
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

    n_try = 0;
    do {
      write(fd, &frame, 5);

      printf("DISC sent.\n");
      alarm(TIMEOUT);
      alrmSet = FALSE;
      while (!alrmSet && state != STOP_S) {
        alarm(TIMEOUT);
        read(fd, &buffer, 1);

        state_machine(&state, buffer, &fields);
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
      if (write(fd, &frame, 5) > 0)
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

    printf("\nReading DISC.\n");

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

    printf("Number of retransmited frames: %d \n", n_retrans);
    printf("Number of duplicated frames: %d \n", n_dups);

  }


  printf("Successfully closed link.\n");
  sleep(1);
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    return SETUP_ERROR;
  }
  close(fd);
  return 0;
}
