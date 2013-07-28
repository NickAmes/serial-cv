PROGRAMMER:= usbtiny
PROGRAMMER_PORT:= usb

CC:=avr-gcc
OBJCOPY:=avr-objcopy
AVRDUDE:=avrdude
CFLAGS:= -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wextra -Wall -Wstrict-prototypes -std=gnu99
LIBS:=
MCU:= attiny25

#Fuse Settings
LFUSE:=0xE2
HFUSE:=0xDC
EFUSE:=0xFF

# Source filename, without extension
FILE:=serialcv_firmware


.PHONY: clean fuses download

# By default, burn fuses, compile, and download.
all: fuses $(FILE).bin download

# Burn fuses
fuses:
	@$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PROGRAMMER_PORT) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

# Compile & link
$(FILE).bin: $(FILE).c
	@$(CC) -mmcu=$(MCU) $(CFLAGS) -I. $(FILE).c -o $(FILE).elf $(LIBS)
	@$(OBJCOPY) -O binary -R .eeprom $(FILE).elf $(FILE).bin

# Download firmware, without changing fuses or eeprom.
download:
	@$(AVRDUDE) -qq -p $(MCU) -c $(PROGRAMMER) -P $(PROGRAMMER_PORT) -U flash:w:$(FILE).bin:r

#Delete generated files
clean:
	@rm -f *.hex *.elf *.bin