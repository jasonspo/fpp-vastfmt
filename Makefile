include /opt/fpp/src/makefiles/common/setup.mk
include /opt/fpp/src/makefiles/platform/*.mk

all: libfpp-vastfmt.so
debug: all

CFLAGS+=-I. -I./vastfmt -I/usr/include/libusb-1.0
OBJECTS_fpp_vastfmt_so += src/FPPVastFM.o  src/hid.o src/Si4713.o src/bitstream.o src/VASTFMT.o src/I2CSi4713.o
LIBS_fpp_vastfmt_so += -L/opt/fpp/src -lfpp -lusb-1.0
CXXFLAGS_src/FPPVastFM.o += -I/opt/fpp/src


%.o: %.cpp Makefile /usr/include/libusb-1.0/libusb.h
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

%.o: %.c Makefile
	$(CCACHE) gcc $(CFLAGS) $(CFLAGS_$@) -c $< -o $@

libfpp-vastfmt.so: $(OBJECTS_fpp_vastfmt_so) /opt/fpp/src/libfpp.so
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_vastfmt_so) $(LIBS_fpp_vastfmt_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-vastfmt.so $(OBJECTS_fpp_vastfmt_so)


/usr/include/libusb-1.0/libusb.h:
	sudo apt-get -q -y update
	sudo apt-get -q -y --reinstall install libusb-1.0-0-dev
