#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "./link_protocol.h"
#include "./state_machine_frame.h"

#include <unistd.h>

struct termios oldtio;
struct Message SET;
struct Message DISC;
struct Message UA;

int fd;
int n_try = 0;
int alrmSet = FALSE;


void alarmHandler(int sig)  {
    alrmSet = TRUE;
    n_try++;
	printf("Alarm nº%d\n",n_try);
}

void alarmHandlerR(int sig)  {
	printf("Timeout. exiting ...\n");
    exit(0);
}

int send_SET() {
	return write(fd, &SET, sizeof(struct Message));
}

int send_DISC() {
	return write(fd, &DISC, sizeof(struct Message));
}

int send_UA() {
	return write(fd, &DISC, sizeof(struct Message));
}

unsigned char bcc_calc(unsigned char a, unsigned char c) {
	return a^c;
}

int llopen(int port, int flag) {
    char port_path [MAX_BUFF];
    struct termios newtio;
    struct Message UA;
    struct Header_Fields header;
    unsigned char aux; 
    int state = 0;
    int i = 0;
    
    signal(SIGALRM,alarmHandler);

    if(port < 0 || port > 2) {
        return INVALID_PORT;
    }
    
    if(flag != TRANSMITTER && flag != RECEIVER) {
        return INVALID_ACTOR;
    }
    
    sprintf(port_path,"/dev/ttyS%d",port);

    fd = open(port_path, O_RDWR | O_NOCTTY );
    if (fd <0) {
        perror(port_path); 
        exit(-1); 
    }

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

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("Serial port set up.\n");

    if(flag == TRANSMITTER) {
        //FILL SET FRAME
        SET.flag_i = SET.flag_f = FLAG;
        SET.a = A_SENDER;
        SET.c = C_SET;
        SET.bcc = bcc_calc(SET.a,SET.c);
        header.A_EXCT = A_SENDER;
        header.C_EXCT = C_UA;

        do {
            send_SET();
            alarm(TIMEOUT); 
            alrmSet=FALSE;              
            i=0;
            while (!alrmSet && state != STOP_S) {
                read(fd,&aux,1);
                state_machine(&state, aux, &header);
                i++;
            }

            if(state == STOP_S)
                break;
        } while(n_try < MAX_RETRIES);

        if(n_try == MAX_RETRIES)
            return TIMEOUT_ERROR;
    }
        
    if (flag == RECEIVER) {
        header.A_EXCT = A_SENDER;
        header.C_EXCT = C_SET;
        alarm(TIMEOUT);
        while (state != STOP_S) {
            if(alrmSet) {
                return TIMEOUT_ERROR;
            }

            read(fd,&aux,1);
            state_machine(&state, aux, &header);
            i++;
        }
        //FILL UA FRAME
        UA.flag_i = UA.flag_f = FLAG;
	    UA.a = A_SENDER;
	    UA.c = C_UA;
	    UA.bcc = bcc_calc(UA.a,UA.c);

        write(fd,&UA,sizeof(struct Message));
    }
    
    return fd;
}

int llwrite(int fd, char *buffer, int length) {
    int n_written;

    return n_written;
}

int main() {
    //just for compilation
    return 0;
}


volatile int STOP=FALSE;

int llread(int fd, char *info)
{
    //info - info in packets of I
    unsigned char buffer[256];
    int i = 0 ;
    int res = 0;
    int state = 0;

    struct Message message;

    signal(SIGALRM,alarmHandlerR);

    while (STOP==FALSE)
    {
      alarm(TIMEOUT_R);
      res = read(fd,buffer[i],1);
      if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
      //state_machine(&state, &buffer[i], message);
      i++;
    }

    write(fd,&message, sizeof(message)); 
    
    //if rr return 0
    //else rej return 1

    return 0;
}

int llclose(int fd,int flag)
{
    int i = 0;
    unsigned char *buffer;

    if(flag == TRANSMITTER)
    {
        signal(SIGALRM,alarmHandler);

        DISC.a = A_SENDER;
        DISC.c =  C_DISC;
        DISC.bcc = bcc_calc(DISC.a,DISC.c);
        DISC.flag_i = DISC.flag_f = FLAG;

        //implement several tries
        send_DISC();

        while (STOP==FALSE)
        {
        alarm(TIMEOUT);
        read(fd,buffer[i],1);
        if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
        //state_machine(&state, &buffer[i], message);
        i++;
        }

        UA.a = A_SENDER;
        UA.c = C_UA;
        UA.bcc = bcc_calc(UA.a,UA.c);
        UA.flag_i = UA.flag_f = FLAG;

        send_UA();

    }

    if(flag == RECEIVER)
    {
        signal(SIGALRM,alarmHandlerR);

        while (STOP==FALSE)
        {
        alarm(TIMEOUT_R);
        read(fd,buffer[i],1);
        if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
        //state_machine(&state, &buffer[i], message);
        i++;
        }

        DISC.a = A_SENDER;
        DISC.c =  C_DISC;
        DISC.bcc = bcc_calc(DISC.a,DISC.c);
        DISC.flag_i = DISC.flag_f = FLAG;

        //implement several tries
        write(fd, &DISC, sizeof(struct Message));
        i =0;
        while (STOP==FALSE)
        {
        alarm(TIMEOUT_R);
        read(fd,buffer[i],1);
        if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
        //state_machine(&state, &buffer[i], message);
        i++;
        }
        
    }

    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    	perror("tcsetattr");
    	exit(-1);
    }
    close(fd);
    return 0;
}

