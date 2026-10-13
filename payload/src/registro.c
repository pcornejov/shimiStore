/* Log en /data/shimistore/log.txt (y en la salida estándar) y avisos en pantalla de la PS5.
 * Copyright (C) 2026 Pablo Cornejo. GPL-3.0-or-later.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "shimi.h"

typedef struct {
  char inutil[45];
  char mensaje[3075];
} aviso_t;

int sceKernelSendNotificationRequest(int dispositivo, aviso_t *aviso, size_t tam, int nada);

static void escribir(const char *texto) {
  static FILE *f;
  if (!f) {
    mkdir(SHIMI_DATOS, 0777);
    f = fopen(SHIMI_DATOS "/log.txt", "a");
  }
  char hora[32];
  time_t t = time(NULL);
  strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", localtime(&t));
  printf("[shimi] %s\n", texto);
  if (f) {
    fprintf(f, "%s %s\n", hora, texto);
    fflush(f);
  }
}

void shimi_log(const char *fmt, ...) {
  char texto[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(texto, sizeof(texto), fmt, args);
  va_end(args);
  escribir(texto);
}

void shimi_aviso(const char *fmt, ...) {
  aviso_t aviso;
  memset(&aviso, 0, sizeof(aviso));
  va_list args;
  va_start(args, fmt);
  vsnprintf(aviso.mensaje, sizeof(aviso.mensaje), fmt, args);
  va_end(args);
  sceKernelSendNotificationRequest(0, &aviso, sizeof(aviso), 0);
  escribir(aviso.mensaje);
}
