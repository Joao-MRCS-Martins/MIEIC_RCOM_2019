CC=gcc
CFLAGS = -Wall -Wextra -pedantic -Wno-pointer-sign
DEPS = defines.h link_protocol.h state_machine_frame.h stuffing.h
OBJ = link_protocol.o state_machine_frame.o stuffing.o
#RECEIVER = 
#EMITTER = 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

link: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o 
		
format:
	clang-format -i *.c *.h -style=llvm

