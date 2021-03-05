CROSS_COMPILE = arm-himix200-linux-

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

CFLAGS =  -Wall -I./include

LINKFLAGS =

LINKLIBS = -lpthread -L./ -lttydetect -L./libusb -lusb-1.0

tty_detect_srcs := usb_tty_detect.c 
tty_detect_objs := $(patsubst %.c, %.o, $(tty_detect_srcs))

tty_detect_static_objs := $(tty_detect_objs)
tty_detect_shared_objs := $(tty_detect_objs)

# tty_detect_static_lib := libttydetect.a
tty_detect_shared_lib := libttydetect.so


tty_detect_sample_srcs := main.c 
tty_detect_sample_objs := $(patsubst %.c, %.o, $(tty_detect_sample_srcs))


tty_detect_sample = ttydetect 

all: $(tty_detect_shared_lib) $(tty_detect_sample)

$(tty_detect_shared_lib): $(tty_detect_shared_objs)
	$(CC) $^ -shared   $(LINKFLAGS) -o $@

$(tty_detect_shared_objs): %.o:%.c 
	$(CC) -c -fPIC $<  $(CFLAGS) $(INC) -o $@



$(tty_detect_sample): $(tty_detect_sample_objs)
	$(CC) $^ $(LINKLIBS) -o $@

$(tty_detect_sample_objs): %.o:%.c 
	$(CC) -c $<  $(CFLAGS) $(INC) -o $@

.PHONY:clean
clean:
	-rm $(tty_detect_shared_objs)
	-rm $(tty_detect_shared_lib)
	-rm $(tty_detect_sample_objs)
	-rm $(tty_detect_sample)



FORCE:
