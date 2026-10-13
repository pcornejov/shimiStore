# shimistore.elf: payload de shimiStore para la PS5 (ps5-payload-sdk de John Törnblom).
#
#   make                       compila shimistore.elf
#   make enviar PS5=<ip>       lo manda al elfldr de la consola (puerto 9021)

PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

PS5  ?= 192.168.68.114
ELF  := shimistore.elf
SRCS := $(wildcard src/*.c)

CFLAGS  := -Os -Wall -Werror -ffunction-sections -fdata-sections -Isrc
LDFLAGS := -Wl,--gc-sections
LIBS    := -lSceAppInstUtil

all: $(ELF)

$(ELF): $(SRCS) $(wildcard src/*.h) $(wildcard assets/*)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LIBS)
	$(STRIP) $@

enviar: $(ELF)
	python3 ../herramientas/enviar.py $(PS5) $(ELF)

clean:
	rm -f $(ELF)

.PHONY: all enviar clean
