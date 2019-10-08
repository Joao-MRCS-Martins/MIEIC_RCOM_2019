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

struct Message {
    unsigned char flag_i;
    unsigned char a;
    unsigned char c;
    unsigned char bcc;
    unsigned char flag_f;
};

volatile int UA_RCV=FALSE;
volatile int STOP=FALSE;
volatile int n_try =0;
int fd;
struct Message SET;
int send_SET();

void alarmHandler()  {
	if(UA_RCV)
		return;
	if(n_try < MAX_RETRIES) {
		int res = send_SET();
		alarm(TIMEOUT);
		n_try++;
	}
	else {
		printf("MAX TRIES REACHED. EXITING...\n");
		exit(0);
	}
}

unsigned char bcc_calc(unsigned char a, unsigned char c) {
	return a^c;
}

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
		  if( info == FLAG) {
		message->flag_f = info;
		    *state = STOP_S;
		  }
		  else
		    *state = START_S;
		  break;

		case STOP_S:
		  break;

		default:
		  *state = START_S;
  }
}

int send_SET() {
	return write(fd, &SET, sizeof(struct Message));
}

int main(int argc, char** argv)
{
    int res;
    struct termios oldtio,newtio;
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
 	
	signal(SIGALRM,alarmHandler);

	SET.flag_i = SET.flag_f = FLAG;
	SET.a = A;
	SET.c = C_SET;
	SET.bcc = bcc_calc(A,C_SET);
	
	res = send_SET();
	n_try++;
	alarm(TIMEOUT);
  	printf("%d bytes written\n", res);


	unsigned char reply[5];
  	i = 0;
  	int state = 0;
	struct Message UA;
	
	
	while (STOP==FALSE)
  	{
   	 res = read(fd,&reply[i],1);
   	 if (reply[4] == FLAG)  STOP=TRUE;
   	 state_machine(&state, reply[i], &UA);
         i++;
  	}

	UA_RCV = TRUE;


    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    	perror("tcsetattr");
    	exit(-1);
    }

    close(fd);
    return 0;
}
