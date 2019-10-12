#ifndef _STATE_MACHINE_FRAME_H
#define _STATE_MACHINE_FRAME_H

#include "./defines.h"
#include <unistd.h>

struct Header_Fields {
    //The A & C values read 
	unsigned char A_READ;
	unsigned char C_READ;
    // The A & C values expected
    unsigned char A_EXCT;
    unsigned char C_EXCT;
};

void state_machine(int *state, unsigned char info, struct Header_Fields *message);

#endif 