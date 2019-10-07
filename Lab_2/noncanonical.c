/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define C_SET 0b00000011
#define C_UA 0b00000111
#define A 0x03
#define FLAG 0x7E

#define START_S 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define STOP_S 5

volatile int STOP=FALSE;
//struct Message msg_rcv;

/*
struct Message {
    unsigned char a;
    unsigned char c;
    unsigned char flag;
    unsigned char bcc;
} msg_rcv;

void state_machine(int *state, unsigned char info)
{
  switch (state) {
    case START_S:
      if(info == FLAG)
      {
        msg_rcv.flag = info
        state = FLAG_RCV;
      }
      else if (info == FLAG)
        state = FLAG_RCV;
      else
        state = START_S;
      break;
    case FLAG_RCV:
      if(info == A)
      {
        message_rcv.a = info;
        state = A_RCV;
      }
      else if (info == FLAG)
        state = FLAG_RCV;
      else
        state = START_S;
      break;
    case A_RCV:
      if(info == C_SET)
      {
        message_rcv.c = info;
        state = C_RCV;
      }
      else if (info == FLAG)
        state = FLAG_RCV;
      else
        state = START_S;
      break;
    case C_RCV:
      if(info == msg_rcv.a^msg_rcv.c )
      {
        msg_rcv.bcc = info;
        state = BCC_RCV;
      }
      else if (info == FLAG)
        state = FLAG_RCV;
      else
        state = START_S;
      break;
    case BCC_RCV:
      if( info == FLAG)
      state =STOP_S;
      else
      state = START_S;
      break;
    case STOP_S:
      break;
    default:
      state = START_S;

  }
}

*/
unsigned char bcc_calc(unsigned char a, unsigned char c) {
  return a^c;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[5];

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

    printf("New termios structure set\n");

    int i = 0 ;
    int state = 0;
//READ MESSAGE
    while (STOP==FALSE)
    {
      res = read(fd,&buf[i],1);
      printf("%d\n",res);

      if (buf[4] == FLAG)  STOP=TRUE;
      printf("received %u \n", buf[i]);
      i++;
    //  state_machine(state,buf[i]);
    }







//WRITE ANSWER

    write(fd,buf,strlen(buf)+1);

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;

  }
