include $(CONFIG_DIR)/config.mk

# C sources
C_SOURCES += \
src/wdsp.c \
boards/$(BOARD)/main.c \
cores/$(CORE)/src/system.c \
cores/$(CORE)/src/io.c\
cores/$(CORE)/src/audio.c

ifeq ($(CONFIG_MODULES_USB), true)
	C_SOURCES += \
	src/usb.c \
	src/usb_config.c \
	cores/$(CORE)/src/usb_phy.c
endif

ifeq ($(CONFIG_MODULES_MIDI), true)
	C_SOURCES += src/midi.c

	ifeq ($(CONFIG_MIDI_UART), true)
		C_SOURCES += cores/$(CORE)/src/midi_uart.c
	endif

	ifeq ($(CONFIG_MODULES_USB), true)
	ifeq ($(CONFIG_MIDI_USB), true)
		C_SOURCES += src/usb_uac.c
		C_SOURCES += src/midi_usb.c
	endif
	endif
endif

C_INCLUDES += \
-I boards/$(BOARD)