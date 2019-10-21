#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "./app_layer.h"
#include "./link_protocol.h"

unsigned char N_SEQ = 0;
int n =0;
char *getFileData(char *filename, int *file_size) {
  FILE *f;
  struct stat meta;

  if ((f = fopen(filename, "r")) == NULL) {
    perror(filename);
    return NULL;
  }

  if (stat(filename, &meta) < 0) {
    perror(filename);
    return NULL;
  }

  *file_size = meta.st_size;
  char *file_data = (char *)malloc(*file_size);

  fread(file_data, sizeof(char), *file_size, f);
  fclose(f);
  return file_data;
}

void makeControlPacket(char *filename, int size, int *packet_size,
                       unsigned char *packet) {
  packet[0] = C_START;
  packet[1] = T_SIZE;
  packet[2] = L1_S;
  packet[3] = (size >> 24) & 0xFF;
  packet[4] = (size >> 16) & 0xFF;
  packet[5] = (size >> 8) & 0xFF;
  packet[6] = size & 0xFF;
  packet[7] = T_NAME;
  packet[8] = strlen(filename);
  strncpy(&packet[9], filename, strlen(filename) + 1);
  *packet_size = 9 + strlen(filename);
}

int makeDataPacket(char *data, int *index, unsigned char *packet,
                   int *packet_size, int data_size) {

  if (*index >= data_size - 1) {
    return DATA_PCKT;
  }

  if (*index + MAX_PCKT_SIZE > data_size) {
    *packet_size = data_size - *index + 4;
  } else {
    *packet_size = MAX_PCKT_SIZE;
  }
  packet[0] = C_DATA;
  packet[1] = N_SEQ;
  packet[2] = (*packet_size - 4) / 256;
  packet[3] = (*packet_size - 4) % 256;
  for (int i = 0; i < *packet_size - 4; i++) {
    packet[4 + i] = data[*index + i];
  }
  N_SEQ = (N_SEQ + 1) % 255;
  *index += *packet_size - 4;
  return 0;
}

int senderApp(int port, char *file) {
  int data_size;
  char *data = getFileData(file, &data_size);
  if (data == NULL) {
    printf("Failed to retrieve file data.\n");
    return FILE_ERROR;
  }

  int c_packet_size;
  unsigned char c_packet[9 + strlen(file)];
  makeControlPacket(file, data_size, &c_packet_size, c_packet);

  // open connection (llopen)
  int fd;
  if ((fd = llopen(port, TRANSMITTER)) < 0) {
    printf("Failed to open connection with receiver.\n");
    return CONNECT_FAIL;
  }

  // send start control packet
  if (llwrite(fd, c_packet, c_packet_size) < 0) {
    printf("Failed to send start packet.\n");
    close(fd);
    return STRT_PCKT;
  }

  int index = 0;
  unsigned char d_packet[MAX_PCKT_SIZE];
  int d_packet_size;
  while (1) {
    if (makeDataPacket(data, &index, d_packet, &d_packet_size, data_size) < 0) {
      break;
    }
    if (llwrite(fd, d_packet, d_packet_size) < 0) {
      printf("Failed to send data packet.\n");
      free(data);
      return DATA_PCKT;
    }
  }
  // send end control packet
  c_packet[0] = C_END;
  if (llwrite(fd, c_packet, c_packet_size) < 0) {
    printf("Failed to send end packet.\n");
    return END_PCKT;
  }

  // close connection (llclose)
  free(data);
  if (llclose(fd, TRANSMITTER) < 0) {
    close(fd);
    printf("Failed to close connection properly with receiver.\n");
    return CONNECT_FAIL;
  }

  return 0;
}

int getStartInfo(int fd, char *filename, int *file_size,
                 unsigned char *c_packet) {
  if (llread(fd, c_packet) < 0) {
    return STRT_PCKT;
  }

  if (c_packet[0] != C_START) {
    printf("Invalid stage: %d. Expected stage: %d\n", c_packet[0], C_START);
    return STRT_PCKT;
  }

  if (c_packet[1] != T_SIZE) {
    printf("Invalid Type: %d. Expected Type: %d\n", c_packet[1], T_SIZE);
    return STRT_PCKT;
  }

  if (c_packet[2] != L1_S) {
    printf("Invalid L1 size: %d. Expected size: %d\n", c_packet[2], L1_S);
    return STRT_PCKT;
  }

  *file_size = c_packet[3] << 24;
  *file_size |= c_packet[4] << 16;
  *file_size |= c_packet[5] << 8;
  *file_size |= c_packet[6];

  if (c_packet[7] != T_NAME) {
    printf("Invalid Type: %d. Expected Type: %d\n", c_packet[7], T_NAME);
    return STRT_PCKT;
  }

  strncpy(filename, &c_packet[9], c_packet[8]);
  return 0;
}

int getPacketInfo(int port_fd, int dest_fd, int *total_read) {
  unsigned char d_packet[2 * MAX_PCKT_SIZE + 4] = "";

  if (llread(port_fd, d_packet) < 0) {
    printf("Failed to read data packet.\n");
    return DATA_PCKT;
  }

  if (d_packet[0] != C_DATA) {
    printf("Invalid packet type: %d. Expected: %d\n", d_packet[0], C_DATA);
    return DATA_PCKT;
  }

  if (d_packet[1] != N_SEQ) {
    printf("Packet out of sequence: %d. Expected: %d\n", d_packet[1], N_SEQ);
    return DATA_PCKT;
  }

  int k = (d_packet[2] << 8) + d_packet[3];
  if (write(dest_fd, &d_packet[4], k) < 0) {
    printf("Failed to write data into destination file.\n");
    return DATA_PCKT;
  }
  printf("nseq %d\n",N_SEQ);
  *total_read += k;
  N_SEQ = (N_SEQ + 1) % 255;
  printf("n_pacote: %d\n", ++n);
  return 0;
}

int checkEndInfo(int fd, unsigned char *c_packet) {
  unsigned char e_packet[MAX_BUFF + 9];
  if (llread(fd, e_packet) < 0) {
    return END_PCKT;
  }

  if (e_packet[0] != C_END) {
    printf("Invalid stage: %d. Expected stage: %d\n", e_packet[0], C_END);
    return END_PCKT;
  }

  if (strcmp(&e_packet[1], &c_packet[1]) != 0) {
    printf("End control packet doesn't match start control packet. Errors in "
           "data expected.\n");
    return END_PCKT;
  }

  return 0;
}

int receiverApp(int port) {
  // open connection (llopen)
  int fd;
  if ((fd = llopen(port, RECEIVER)) < 0) {
    printf("Failed to open connection with receiver.\n");
    return CONNECT_FAIL;
  }

  // get control packet info to start
  char filename[MAX_BUFF] = "";
  int file_size;
  unsigned char c_packet[MAX_BUFF + 9];
  if (getStartInfo(fd, filename, &file_size, c_packet) < 0) {
    close(fd);
    return STRT_PCKT;
  }

  // open/create file indicated in start control packet
  int dest_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC);
  if (dest_fd < 0) {
    printf("Failed to create new file.\n");
    close(fd);
    return FILE_ERROR;
  }

  // receive packets and process them into file data
  int rcv_bytes = 0;
  while (rcv_bytes != file_size) {
    if (getPacketInfo(fd, dest_fd, &rcv_bytes) < 0) {
      printf("Error receiving package.Terminating.\n");
      close(dest_fd);
      close(fd);
      return DATA_PCKT;
    }
    // printf("N seq: %d Rcv bytes: %d bytes left:
    // %d\n",N_SEQ,rcv_bytes,file_size-rcv_bytes);
  }
  // receive end control packet and validate with start packet
  if (checkEndInfo(fd, c_packet) < 0) {

    close(fd);
    return END_PCKT;
  }

  // close connection (llclose)
  if (llclose(fd, RECEIVER) < 0) {
    close(fd);
    printf("Failed to close connection properly with receiver.\n");
    return CONNECT_FAIL;
  }
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 4 && argc != 3) {
    printf("Wrong number of arguments. Usage: ./link "
           "<TRANSMITTER|RECEIVER>[0|1] <SERIAL PORT>[0|1|2] <file>\n");
    return INVALID_ARGS;
  }

  int port = atoi(argv[2]);
  printf("port: %d\n", port);
  if (port < 0 || port > 5) {
    printf("Wrong Port. Use ports 0, 1 or 2.\n");
    return INVALID_PORT;
  }

  int actor = atoi(argv[1]);
  printf("actor: %d\n", actor);
  if (actor == TRANSMITTER) {
    return senderApp(port, argv[3]);
  } else if (actor == RECEIVER) {
    return receiverApp(port);
  } else {
    printf("Invalid actor.\n");
    return INVALID_ACTOR;
  }
}
