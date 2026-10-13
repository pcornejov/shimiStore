/* shimiStore: tienda de juegos que corre en la PS5.
 * Copyright (C) 2026 Pablo Cornejo. GPL-3.0-or-later (ver LICENSE).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define SHIMI_VERSION "0.1.0"
#define SHIMI_NOMBRE "shimistore.elf" /* nombre del hilo: para encontrar una copia anterior */
#define SHIMI_PUERTO 8090             /* igual que deeplinkUri en assets/param.json */
#define SHIMI_TITLE_ID "SHMI00001"
#define SHIMI_DATOS "/data/shimistore"

/* Incrusta un archivo de assets/ en el .elf (como en Payload Manager). */
#define INCASSET(name, file)                                                   \
  __asm__(".section .rodata\n"                                                 \
          ".global " #name "\n"                                                \
          ".global " #name "_end\n"                                            \
          ".global " #name "_size\n"                                           \
          ".align 16\n" #name ":\n"                                            \
          ".incbin \"" file "\"\n" #name "_end:\n" #name "_size:\n"            \
          ".quad " #name "_end - " #name "\n"                                  \
          ".previous\n");                                                      \
  extern const uint8_t name[];                                                 \
  extern const size_t name##_size;

/* registro.c */
void shimi_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void shimi_aviso(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* icono.c */
int shimi_instalar_icono(void);

/* servidor.c */
int shimi_servidor(int puerto, volatile int *seguir);
