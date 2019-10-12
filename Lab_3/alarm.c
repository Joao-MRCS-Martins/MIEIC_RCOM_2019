#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "./alarm.h"
#include "./link_protocol.h"
#include "./defines.h"

int n_try = 0;
int send = 0;

void alarmHandler(int sig)  {
	if(n_try < MAX_RETRIES) {
		switch(sig)
		{
			case 0:
				send_SET();
				break;
			case 1:
				send_DISC();
				break;
			case 2: 
				send_UA();
				break;
		}
		alarm(TIMEOUT);
		n_try++;
	}
	else {
		printf("MAX TRIES REACHED. EXITING...\n");
		exit(0);
	}
}

void alarmHandlerR()  {
	
	printf("TIMEOUT. EXITING...\n");
	exit(0);

}