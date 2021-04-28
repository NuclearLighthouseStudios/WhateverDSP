# C sources
C_SOURCES += \
src/midi.c \
boards/$(BOARD)/src/main.c \
cores/$(CORE)/src/system.c \
cores/$(CORE)/src/io.c\
cores/$(CORE)/src/audio.c\
cores/$(CORE)/src/midi_uart.c

C_INCLUDES += \
-I boards/$(BOARD)