#include "./state_machine_frame.h"
#include <stdio.h>

int i = 0;
unsigned char aux1 = '0';
unsigned char aux2 = '0';

void state_machine(int *state, unsigned char info,
                   struct header_fields *message) {
  switch (*state) {
  case START_S:
    if (info == FLAG) {
      *state = FLAG_RCV;
    } else
      *state = START_S;
    break;

  case FLAG_RCV:
    if (info == message->A_EXCT) {
      message->A_READ = info;
      *state = A_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;

  case A_RCV:
    if (info == message->C_EXCT) {
      message->C_READ = info;
      *state = C_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;

  case C_RCV:
    if (info == (message->A_READ ^ message->C_READ)) {
      *state = BCC_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;
  case BCC_RCV:
    if (info == FLAG) {
      alarm(0);
      *state = STOP_S;
    } else
      *state = START_S;
    break;
  case STOP_S:
    break;

  default:
    *state = START_S;
  }
}

void state_machine_I(int *state, unsigned char info, unsigned char *packets,
                     unsigned char *bcc_data, int C) {

  switch (*state) {
  case START_S:
    if (info == FLAG) {
      *state = FLAG_RCV;
    } else
      *state = START_S;
    break;

  case FLAG_RCV:
    if (info == A_SENDER) {
      *state = A_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;

  case A_RCV:
    if (info == C_S0 || info == C_S1) {
      C = info;
      *state = C_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;

  case C_RCV:
    if (info == (A_SENDER ^ C)) {
      *state = BCC_RCV;
    } else if (info == FLAG)
      *state = FLAG_RCV;
    else
      *state = START_S;
    break;
  case BCC_RCV:
    if (info != FLAG && info != 0) {
      aux1 = info;
      i++;
      *state = INFO;
    }
    break;
  case INFO:
    if (info != FLAG) {
      if (i % 2 == 0) {
        packets[i - 2] = aux1;
        packets[i - 1] = aux2;
        aux1 = info;
      } else {
        aux2 = info;
      }
      i++;
    } else {

      if (aux1 == ESCAPE) {
        bcc_data[0] = aux1;
        bcc_data[1] = aux2;
      } else if (aux2 != 0) {
        bcc_data[0] = aux2;
        packets[i - 1] = aux1;
      } else {
        bcc_data[0] = aux1;
      }

      printf("hkgdfkh\n");
      *state = STOP_I;
    }
    break;
  case STOP_I:
    break;

  default:
    *state = START_S;
  }
}
