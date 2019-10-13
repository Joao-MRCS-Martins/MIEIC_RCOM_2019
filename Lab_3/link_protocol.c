#include "./link_protocol.h"
#include "./state_machine_frame.h"
#include "./stuffing.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

struct termios oldtio;
struct control_frame message;

int fd;
int n_try = 0;
int alrmSet = FALSE;
int n_seq = 0;


void alarmHandler()  {
    alrmSet = TRUE;
    n_try++;
    printf("Alarm nº%d\n", n_try);
}

void alarmHandlerR()  {
	printf("Timeout. exiting ...\n");
    exit(0);
}

/*this needs to change to a generic one*/
int send_message()
{
    return write(fd, &message, sizeof(struct control_frame));
}

unsigned char bcc_calc(unsigned char a, unsigned char c)
{
    return a ^ c;
}

unsigned char* bcc2_calc( char* message, int length) {
    unsigned char *bcc2 = (unsigned char *) malloc(sizeof(unsigned char));
    *bcc2 = message[0];
    for (int i = 1; i < length; i++) {
        *bcc2 ^= message[i];
    }

    return bcc2;
}

int llopen(int port, int flag)
{
    char port_path[MAX_BUFF];
    struct termios newtio;
    struct control_frame UA;
    struct Header_Fields header;
    unsigned char aux;
    int state = 0;
    int i = 0;

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
    newtio.c_cc[VMIN] = 5; /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("Serial port set up.\n");

    if (flag == TRANSMITTER) {
        //FILL SET FRAME
        message.flag_i = message.flag_f = FLAG;
        message.a = A_SENDER;
        message.c = C_SET;
        message.bcc = bcc_calc(message.a, message.c);
        header.A_EXCT = A_SENDER;
        header.C_EXCT = C_UA;

        do {
            send_message();
            alarm(TIMEOUT);
            alrmSet = FALSE;
            i = 0;
            while (!alrmSet && state != STOP_S) {
                read(fd, &aux, 1);
                state_machine(&state, aux, &header);
                i++;
            }

            if (state == STOP_S)
                break;
        } while (n_try < MAX_RETRIES);

        if (n_try == MAX_RETRIES)
            return TIMEOUT_ERROR;
    }

    if (flag == RECEIVER) {
        header.A_EXCT = A_SENDER;
        header.C_EXCT = C_SET;
        alarm(TIMEOUT);
        while (state != STOP_S) {
            if (alrmSet) {
                return TIMEOUT_ERROR;
            }

            read(fd, &aux, 1);
            state_machine(&state, aux, &header);
            i++;
        }
        //FILL UA FRAME
        message.flag_i = UA.flag_f = FLAG;
        message.a = A_SENDER;
        message.c = C_UA;
        message.bcc = bcc_calc(message.a, message.c);

        write(fd, &message, sizeof(struct control_frame));
    }

    return fd;
}

int llwrite(int fd, char *buffer, int length) {
    //int n_written;
    struct info_frame message;

    //setup frame header
    message.flag_i = FLAG;
    message.a = A_SENDER;
    if (n_seq == 0) //check sequence number
        message.c = C_S0;
    else
        message.c = C_S1;
    message.bcc1 = message.a ^ message.c;

    //bcc2 generation & stuffing
    unsigned char* bcc2 = bcc2_calc(buffer, length);
    unsigned char* bcc2_stuffed = bcc2_stuffing(bcc2);
    message.bcc2 = bcc2_stuffed;

    //byte stuffing on file data
    unsigned stuffed_size;
    message.data = data_stuffing(buffer, length, &stuffed_size);
    message.data_size = stuffed_size;

    n_seq ^= 1;     //PLACE WHERE RR IS CORRECTLY RECEIVED

    n_seq ^= 1; //PLACE WHERE RR IS CORRECTLY RECEIVED

    //fazer stuffing do pacote de dados
    //montar frame de informacao (cabecalho dados bcc2 flag)
    write(fd,&message,sizeof(struct info_frame));
    //esperar pela resposta
    //processar resposta
    //se ocorrer timeout ou se receber REJ retransmitir
    //se tentar MAX_RETRIES vezes retornar com erro 
    return 0;
}

volatile int STOP = FALSE;

int llread(int fd, char* info)
{
    unsigned char buffer[256];
    unsigned char packets[256];
    int i = 0;
    int state = 0;
    int flag_answer = 0;

    struct control_frame message;
    struct Header_Fields fields;

    fields.A_EXCT = A_SENDER;
    fields.C_EXCT = C_S0; //can change

    signal(SIGALRM, alarmHandlerR);

    while (state == STOP_I) {
        alarm(TIMEOUT_R);
        read(fd, &buffer[i], 1);
        state_machine_I(&state, buffer[i], &fields, packets, flag_answer);
        i++;
    }

    message.flag_i = message.flag_f = FLAG;
    message.a = A_SENDER;

    if (flag_answer == RR) {
        message.c = RR_R0;
        message.bcc = bcc_calc(message.a, message.c);
    } else {
        message.c = REJ_R0;
        message.bcc = bcc_calc(message.a, message.c);
    }
    write(fd, &message, sizeof(struct control_frame));

    return 0;
}

int llclose(int fd, int flag)
{
    int state = 0;
    unsigned char buffer;

    if (flag == TRANSMITTER) {
        struct Header_Fields fields;

        fields.A_EXCT = A_RECEIVER;
        fields.C_EXCT = C_DISC;

        signal(SIGALRM, alarmHandler);

        message.a = A_SENDER;
        message.c = C_DISC;
        message.bcc = bcc_calc(message.a, message.c);
        message.flag_i = message.flag_f = FLAG;

        do {
            send_message();
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

        message.a = A_SENDER;
        message.c = C_UA;
        message.bcc = bcc_calc(message.a, message.c);
        message.flag_i = message.flag_f = FLAG;

        do {
            if (send_message() < 0)
                alarm(TIMEOUT);
        } while (n_try < MAX_RETRIES);

        if (n_try == MAX_RETRIES)
            return TIMEOUT_ERROR;
    }

    if (flag == RECEIVER) {
        struct Header_Fields fields;

        fields.A_EXCT = A_SENDER;
        fields.C_EXCT = C_DISC; //can change

        signal(SIGALRM, alarmHandlerR);

        while (state != STOP_S) {
            alarm(TIMEOUT_R);
            read(fd, &buffer, 1);
            state_machine(&state, buffer, &fields);
        }

        message.a = A_SENDER;
        message.c = C_UA;
        message.bcc = bcc_calc(message.a, message.c);
        message.flag_i = message.flag_f = FLAG;

        //implement several tries
        write(fd, &message, sizeof(struct control_frame));
        while (state != STOP_S) {
            alarm(TIMEOUT_R);
            read(fd, &buffer, 1);
            state_machine(&state, buffer, &fields);
        }
    }

    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);
    return 0;
}

int main()
{
    unsigned char* bcc2 = (unsigned char*)malloc(sizeof(unsigned char));
    *bcc2 = ESCAPE;
    unsigned char * stuffed = bcc2_stuffing(bcc2);
    if(stuffed == NULL) {
        printf("No stuffing needed. BCC: %s\n",bcc2);
    }
    return 0;
}
