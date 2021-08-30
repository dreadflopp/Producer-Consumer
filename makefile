#   File:	makefile
#   Builds:	Lab4
#   Date:	October 2017
						
CCX = g++

CCFLAGS		=   -ansi -pedantic -std=c++11
#LDFLAGS		= -lrt
LDFLAGS	=  -pthread

PROGS		= Lab4
OBJS		= lab4.o Queue.o Restart.o SharedMem.o 
HDRS		= Queue.h Restart.h SharedMem.h

.cc.o:
	@echo "Compiling " $<
	$(CCX) -c $(CCFLAGS) $<
	@echo $@ "done"

all:	$(PROGS)


Lab4:	$(OBJS) 
	@echo "Linking " $@
	$(CCX)  -o $@ $(OBJS) $(LDFLAGS)
	@echo $@ "done"

clean:
	$(RM) $(PROGS) $(OBJS) core *~
