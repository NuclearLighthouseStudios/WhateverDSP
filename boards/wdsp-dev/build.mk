# C sources
C_SOURCES =  \
boards/$(BOARD)/src/main.c \
cores/$(CORE)/src/system.c \
cores/$(CORE)/src/io.c\
cores/$(CORE)/src/audio.c\
cores/$(CORE)/src/midi.c

# ASM sources
ASM_SOURCES =  \
cores/$(CORE)/startup_stm32f405xx.s

# C includes
C_INCLUDES =  \
-I includes \
-I includes/arm \
-I includes/cmsis \
-I includes/stm