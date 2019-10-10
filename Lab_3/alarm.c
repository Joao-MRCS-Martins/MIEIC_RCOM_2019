#include "alarm.h"

int n_try = 0;

void alarmHandler()  {
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