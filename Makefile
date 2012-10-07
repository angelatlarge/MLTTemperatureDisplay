# AVR-GCC Makefile
PROJECT=temp_display
SOURCES=temp_display.cpp
#TOOLS_DIR=/home/kirill/arduino/arduino-1.0.1-windows/hardware/tools/avr/bin
AVR_DIR=/usr/local/avr
TOOLS_DIR=$(AVR_DIR)/bin
CC=$(TOOLS_DIR)/avr-gcc
# results: 
#	/usr/local/avr/bin/avr-gcc -c  -mmcu=atmega48a -Wall temp_display.c -o temp_display.o
CXX=$(TOOLS_DIR)/avr-g++
# results:
#	/usr/local/avr/bin/avr-g++ -c   temp_display.cpp -o temp_display.o
OBJCOPY=$(TOOLS_DIR)/avr-objcopy
AVRDUDE=/home/kirill/arduino/arduino-1.0/hardware/tools/avr/bin/avrdude.exe
AVRDUDE_CONF	= C:\\cygwin\\home\\kirill\\arduino\\arduino-1.0\\hardware\\tools\\avr\\etc\\avrdude.conf
MCU=atmega48a
MCU_DUDE=m48
ISP_PROG= -c usbtiny
EXTRA_OBJS=$(AVR_DIR)/avr/lib/libm.a


ifndef MCU_DUDE
MCU_DUDE=$(MCU)
endif
AVRDUDE_COM_OPTS = -p $(MCU_DUDE)
ifdef AVRDUDE_CONF
AVRDUDE_COM_OPTS += -C $(AVRDUDE_CONF)
endif
AVRDUDE_ISP_OPTS = $(ISP_POST_OPTION) $(ISP_PROG)
ifndef ISP_PROG
ISP_PROG	   = -c stk500v2
endif
ifdef ISP_PORT
ISP_POST_OPTION = -P $(ISP_PORT)
endif 
AVRDUDE_ISP_OPTS = $(ISP_POST_OPTION) $(ISP_PROG)
REMOVE=rm -f


#####################################################################
# CFLAGS 	= flags for CC
# CXXFLAGS 	= flags for CXX
# CPPFLAGS 	= flags for all compilers

CPPFLAGS	=	\
			-mmcu=$(MCU) \
			-g -Os -w -Wall \
			-ffunction-sections -fdata-sections
CFLAGS       = -std=gnu99
CXXFLAGS     = -fno-exceptions
LDFLAGS       = -mmcu=$(MCU) -Wl,--gc-sections -Os
			
#~ CPPFLAGS      = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DARDUINO=$(ARDUINO_VERSION) \
			#~ $(USER_CPPFLAGS) \
			#~ -I. \
			#~ -I$(ARDUINO_CORE_PATH) \
			#~ -I$(ARDUINO_VAR_PATH)/$(VARIANT) \
			#~ $(SYS_INCLUDES) \
			#~ $(USER_INCLUDES) \
			#~ $(EXTRA_PATHS) \
			#~ -g -Os -w -Wall \
			#~ -ffunction-sections -fdata-sections


#####################################################################

all:	$(PROJECT).hex

ispload: $(PROJECT).hex
	$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) -U flash:w:$(PROJECT).hex
	
clean:
	$(REMOVE) $(PROJECT).out $(PROJECT).o $(PROJECT).hex $(PROJECT).elf
	
$(PROJECT).elf:	$(PROJECT).o
	$(CC) $(LDFLAGS) -o $@ $(PROJECT).o $(EXTRA_OBJS) -lc -lm

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	
%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

	