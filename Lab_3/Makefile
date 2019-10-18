CC=gcc
CFLAGS = -Wall -Wextra -pedantic -Wno-pointer-sign -Wno-pointer-sign
DEPS = defines.h link_protocol.h state_machine_frame.h stuffing.h app_layer.h
OBJ = link_protocol.o state_machine_frame.o stuffing.o app_layer.o
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

