# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h
LDFLAGS		= -L../lib -L/opt/ti/msp430_gcc/include/ 

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c

all: toy.elf

#additional rules for files
toy.elf: ${COMMON_OBJECTS} toy.o wdt_handler.o buzzer.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd -lShape -lCircle -lp2sw

buzzer.o: buzzer.h

load: toy.elf
	mspdebug rf2500 "prog $^"

clean:
	rm -f *.o *.elf *~
