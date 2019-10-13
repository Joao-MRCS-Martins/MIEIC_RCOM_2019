#include "./state_machine_frame.h"


void state_machine(int *state, unsigned char info, struct Header_Fields *message) {
	switch (*state) {
		case START_S:
		  if(info == FLAG) {
		    *state = FLAG_RCV;
		  }
		  else
		    *state = START_S;
		  break;

		case FLAG_RCV:
		  if(info == message->A_EXCT) {
			message->A_READ = info;
		    *state = A_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;

		case A_RCV:
		  if(info == message->C_EXCT) {
			message->C_READ = info;
		    *state = C_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;

		case C_RCV:
		  if(info == (message->A_READ^message->C_READ)) {
		    *state = BCC_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;
		case BCC_RCV:
		    if(info == FLAG) {
			alarm(0);
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

void state_machine_I(int *state, unsigned char info, struct Header_Fields *message ,unsigned char *packets,int answer_flag) {
	int i  = 0;
	unsigned char aux;
	unsigned char bcc_data;
	switch (*state) {
		case START_S:
		  if(info == FLAG) {
		    *state = FLAG_RCV;
		  }
		  else
		    *state = START_S;
		  break;

		case FLAG_RCV:
		  if(info == message->A_EXCT) {
			message->A_READ = info;
		    *state = A_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;

		case A_RCV:
		  if(info == message->C_EXCT) {
			message->C_READ = info;
		    *state = C_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;

		case C_RCV:
		  if(info == (message->A_READ^message->C_READ)) {
		    *state = BCC_RCV;
		  }
		  else if (info == FLAG)
		    *state = FLAG_RCV;
		  else
		    *state = START_S;
		  break;
		case BCC_RCV:
		if(info != FLAG) {
			aux = info;
			i++;
			*state = INFO;
		  }
		case INFO:
		if(info != FLAG) {
			packets[i] = aux;
					aux = info;
			i++;
		}
		/*else
		  {
			//destuffing function
			if(bcc_data == function to calculate bcc of data field)
				if (new frame)
					answer_flag = RR
				else
					answer_flag == REJ
			else
				if(new frame)
					answer_flag = RR
				else
					packets = null;
			*state = STOP_I	
		  }*/
		  break;
		case STOP_I:
		  break;

		default:
		  *state = START_S;
  }
}
