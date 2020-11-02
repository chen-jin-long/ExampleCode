TARGET = thread_pool_test
all: $(TARGET)
CTAGS = -g
SOURCE := $(wildcard *.c) $(wildcard *.cc) 
OBJS := $(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCE)))
$(TARGET):$(OBJS)
	gcc  -o $@ $(OBJS) -lpthread -I . $(CTAGS) 
clean:
	rm *.o
	rm $(TARGET)

.PHONY:all clean
