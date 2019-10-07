/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0b01111110
#define A 0b00000011
#define C_SET 0b00000011
#define C_UA 0b00000111
#define MAX_RETRIES 3
#define TIMEOUT 3

#define START_S 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define STOP_S 5

volatile int STOP=FALSE;
volatile int n_try =0;

void alarmHandler()  {
	//cenas
	if(n_try < MAX_RETRIES) {
		//send_SET();
		alarm(TIMEOUT);
		n_try++;
	}
	else {
		printf("MAX TRIES REACHED. EXITING...\n");
		return;
	}
}

unsigned char bcc_calc(unsigned char a, unsigned char c) {
	return a^c;
}

struct Message {
    unsigned char a;
    unsigned char c;
    unsigned char flag;
    unsigned char bcc;
};

void state_machine(int *state, unsigned char info, struct Message *message)
{
  switch (*state) {
    case START_S:
      if(info == FLAG)
      {
        message->flag = info;
        *state = FLAG_RCV;
      }
      else if (info == FLAG)
        *state = FLAG_RCV;
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
      if(info == C_UA)
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
      if( info == FLAG)
        *state = STOP_S;
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

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char frame[5];
    int i, sum = 0, speed = 0;

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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
 	
	signal(SIGALRM,alarmHandler);

	frame[0] = frame[4] = FLAG;
	frame[1] = A;
	frame[2] = C_SET;
	frame[3] = bcc_calc(A,C_SET);
	
	res = write(fd, frame, sizeof(frame));
	//change to send_SET()
	alarm(TIMEOUT);
  printf("%d bytes written\n", res);


	unsigned char reply[5];
  i = 0;
  int state = 0;
  struct Message msg;

  while (STOP==FALSE)
  {
    res = read(fd,&reply[i],1);
    if (reply[4] == FLAG)  STOP=TRUE;
    state_machine(&state, reply[i], &msg);
    i++;
  }


	//printf("\nthis is buf %s\n", reply);

  /*
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar
    o indicado no gui�o
  */




    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
