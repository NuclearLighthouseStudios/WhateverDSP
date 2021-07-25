# C sources
C_SOURCES += \
src/wdsp.c \
src/midi.c \
src/usb.c \
src/usb_config.c \
src/usb_uac.c \
src/midi_usb.c \
src/audio_usb.c \
boards/$(BOARD)/main.c \
cores/$(CORE)/src/system.c \
cores/$(CORE)/src/io.c\
cores/$(CORE)/src/usb_phy.c\
cores/$(CORE)/src/audio.c\
cores/$(CORE)/src/midi_uart.c

C_INCLUDES += \
-I boards/$(BOARD)