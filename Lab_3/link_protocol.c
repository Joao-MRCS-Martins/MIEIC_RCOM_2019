#include "link_protocol.h"
#include "state_machine_frame.c"

#include <unistd.h>

int main()
{
   struct Message *message;
    
    llread(2,message);
}














































































volatile int STOP=FALSE;

int llread(int fd, struct Message *message)
{
    unsigned char buffer[256];
    int i = 0 ;
    int res = 0;
    int state = 0;

    while (STOP==FALSE)
    {
      res = read(fd,&buffer[i],1);
      if (buffer[i] == FLAG && i != 0)  STOP=TRUE;
      state_machine(&state, buffer[i], message);
      i++;
    }

    

    

    
}

