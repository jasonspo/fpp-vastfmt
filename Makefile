SRCDIR ?= /opt/fpp/src
include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

all: libfpp-vastfmt.$(SHLIB_EXT)
debug: all

ifeq '$(ARCH)' 'OSX'
USBHEADERPATH=$(HOMEBREW)/include/libusb-1.0
$(USBHEADERPATH)/libusb.h:
	brew install libusb
	
OBJECTS_fpp_vastfmt_so+=src/hid-mac.o
LIBS_fpp_vastfmt_so +=-framework IOKit -framework CoreFoundation
else
USBHEADERPATH=/usr/include/libusb-1.0
$(USBHEADERPATH)/libusb.h:
	sudo apt-get -q -y update
	sudo apt-get -q -y --reinstall install libusb-1.0-0-dev
	
OBJECTS_fpp_vastfmt_so+=src/hid.o
endif


CFLAGS+=-I. -I./vastfmt -I$(USBHEADERPATH)
OBJECTS_fpp_vastfmt_so += src/FPPVastFM.o  src/Si4713.o src/bitstream.o src/VASTFMT.o src/I2CSi4713.o
LIBS_fpp_vastfmt_so += -L$(SRCDIR) -lfpp -lusb-1.0 -ljsoncpp
CXXFLAGS_src/FPPVastFM.o += -I$(SRCDIR)



%.o: %.cpp Makefile $(USBHEADERPATH)/libusb.h
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

%.o: %.c Makefile
	$(CCACHE) gcc $(CFLAGS) $(CFLAGS_$@) -c $< -o $@

libfpp-vastfmt.$(SHLIB_EXT): $(OBJECTS_fpp_vastfmt_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_vastfmt_so) $(LIBS_fpp_vastfmt_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-vastfmt.$(SHLIB_EXT) $(OBJECTS_fpp_vastfmt_so)
