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
struct control_frame message;

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

/*this needs to change to a generic one*/
int send_message() {
	return write(fd, &message, sizeof(struct control_frame));
}

unsigned char bcc_calc(unsigned char a, unsigned char c) {
	return a^c;
}

unsigned char bcc2_calc(unsigned char* message, int length) {
    unsigned char bcc2 = message[0];
    for(int i =1;i<length;i++) {
        bcc2 ^= message[i];
    }

    return bcc2;
}

unsigned char* bcc2_stuffing(unsigned char bcc2, int *size) {
    unsigned char* bcc2_stuffed;
    if(bcc2 == FLAG) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
        sprintf(bcc2_stuffed,"%x",FLAG_ESC);
        *size = 2;
    }
    else if(bcc2 == ESCAPE) {
        bcc2_stuffed = (unsigned char *) malloc(2* sizeof(unsigned char *));
       sprintf(bcc2_stuffed,"%x",ESC_ESC);
        *size = 2;
    }
    else {
        *size = 1;
        return NULL;
    }

    return bcc2_stuffed;
}

int llopen(int port, int flag) {
    char port_path [MAX_BUFF];
    struct termios newtio;
    struct control_frame UA;
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
        message.flag_i = message.flag_f = FLAG;
        message.a = A_SENDER;
        message.c = C_SET;
        message.bcc = bcc_calc(message.a,message.c);
        header.A_EXCT = A_SENDER;
        header.C_EXCT = C_UA;

        do {
            send_message();
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
        message.flag_i = UA.flag_f = FLAG;
	    message.a = A_SENDER;
	    message.c = C_UA;
	    message.bcc = bcc_calc(message.a,message.c);

        write(fd,&message,sizeof(struct control_frame));
    }
    
    return fd;
}

int llwrite(int fd, char *buffer, int length) {
    int n_written;
    //montar cabecalho FLAG A C BCC1
    //calcular bcc2 com dados atuais
    //fazer stuffing do pacote de dados
    //montar frame de informacao (cabecalho dados bcc2 flag)
    //enviar
    //esperar pela resposta
    //se ocorrer timeout ou se receber REJ retransmitir
    //se tentar MAX_RETRIES vezes retornar com erro 
    return n_written;
}

volatile int STOP=FALSE;

int llread(int fd, char *info)
{
    //info - info in packets of I
    unsigned char buffer[256];
    unsigned char packets[256];
    int i = 0 ;
    int res = 0;
    int state = 0;
    int flag_answer = 0;

    struct control_frame message;
    struct Header_Fields fields;

    fields.A_EXCT = A_SENDER;
    fields.C_EXCT = C_S0; //can change
    
    signal(SIGALRM,alarmHandlerR);

    while (STOP==FALSE)
    {
      alarm(TIMEOUT_R);
      res = read(fd,&buffer[i],1);
      if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
      state_machine_I(&state, buffer[i], &fields, packets,flag_answer);
      i++;
    }

    message.flag_i = message.flag_f = FLAG;
    message.a = A_SENDER;

    if(flag_answer == RR)
    {
        message.c = RR_R0;
        message.bcc = bcc_calc(message.a,message.c);
    }
    else
    {
        message.c = REJ_R0;
        message.bcc = bcc_calc(message.a,message.c);
    }
        write(fd,&message, sizeof(struct control_frame)); 
    
    
    return 0;
}

int llclose(int fd,int flag)
{
    int i = 0;
    unsigned char *buffer;

    if(flag == TRANSMITTER)
    {
        signal(SIGALRM,alarmHandler);

        message.a = A_SENDER;
        message.c =  C_DISC;
        message.bcc = bcc_calc(message.a,message.c);
        message.flag_i = message.flag_f = FLAG;

        //implement several tries
        send_message();

        while (STOP==FALSE)
        {
        alarm(TIMEOUT);
        read(fd,&buffer[i],1);
        if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
        //state_machine(&state, &buffer[i], message);
        i++;
        }

        message.a = A_SENDER;
        message.c = C_UA;
        message.bcc = bcc_calc(message.a,message.c);
        message.flag_i = message.flag_f = FLAG;

        send_message();

    }

    if(flag == RECEIVER)
    {
        signal(SIGALRM,alarmHandlerR);

        while (STOP==FALSE)
        {
        alarm(TIMEOUT_R);
        read(fd,&buffer[i],1);
        if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
        //state_machine(&state, &buffer[i], message);
        i++;
        }

        message.a = A_SENDER;
        message.c =  C_DISC;
        message.bcc = bcc_calc(message.a,message.c);
        message.flag_i = message.flag_f = FLAG;

        //implement several tries
        write(fd, &message, sizeof(struct control_frame));
        i =0;
        while (STOP==FALSE)
        {
        alarm(TIMEOUT_R);
        read(fd,&buffer[i],1);
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


int main() {
    unsigned char bcc2 = FLAG;
    int size;
    unsigned char * stuffed = bcc2_stuffing(bcc2,&size);
    if(stuffed == NULL) {
        printf("No stuffing needed.\n");
    }
    else {
       printf("BCC stuffed: %s Size: %d\n",stuffed,size);
    }
    return 0;
}
