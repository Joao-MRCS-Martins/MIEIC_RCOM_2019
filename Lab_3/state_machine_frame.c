#include "state_machine_frame.h"

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
		  if(info == A_RECIEVER)
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

