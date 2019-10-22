/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defines.h"

volatile int STOP=FALSE;

struct Message {
	unsigned char flag_i;    
	unsigned char a;
    unsigned char c;
    unsigned char bcc;
    unsigned char flag_f;
};


void state_machine(int *state, unsigned char info, struct Message *message)
{
  switch (*state) {
    case START_S:
      if(info == FLAG)
      {
        message->flag_i = info;
        *state = FLAG_RCV;
      }
      else
        *state = START_S;
      break;

    case FLAG_RCV:
      if(info == A)
      {
        message->a = info;
        *state = A_RCV;
      }
      else if (info == FLAG)
        *state = FLAG_RCV;
      else 
        *state = START_S;
      break;

    case A_RCV:
      if(info == C_SET)
      {
        message->c = info;
        *state = C_RCV;
      }
      else if (info == FLAG)
        *state = FLAG_RCV;
      else
        *state = START_S;
      break;

    case C_RCV:
      if(info == message->a^message->c )
      {
        message->bcc = info;
        *state = BCC_RCV;
      }
      else if (info == FLAG)
        *state = FLAG_RCV;
      else
        *state = START_S;
      break;

    case BCC_RCV:
      if( info == FLAG) {
        *state = STOP_S;
		message->flag_f = info;	
	   }
      else
        *state = START_S;
      break;

    case STOP_S:
      break;

    default:
      *state = START_S;
  }
  printf("%d\n", *state);
}


unsigned char bcc_calc(unsigned char a, unsigned char c) {
  return a^c;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio, newtio;
    unsigned char buf[5];
    unsigned char frame[5];

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

   // printf("New termios structure set\n");

    int i = 0 ;
    int state = 0;
    struct Message msg;
	//READ MESSAGE
    while (STOP==FALSE)
    {
      res = read(fd,&buf[i],1);
	  printf("Received: 0x%04x ",buf[i]);
      if (buf[4] == FLAG)  STOP=TRUE;
      state_machine(&state, buf[i], &msg);
      i++;
    }
    	
	sleep(12);

	//WRITE ANSWER

    frame[0] = frame[4] = FLAG;
    frame[1] = A;
    frame[2] = C_UA;
    frame[3] = bcc_calc(A, C_UA);

    res = write(fd, frame, sizeof(frame));

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;

  }
