/* shimistore.elf: punto de entrada. Cierra una copia anterior, instala el ícono y levanta el servidor web.
 * Copyright (C) 2026 Pablo Cornejo. GPL-3.0-or-later.
 */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include "shimi.h"

/* PID de otro proceso con ese nombre de hilo, o -1 (como find_pid de Payload Manager). */
static pid_t buscar_pid(const char *nombre) {
  int mib[4] = {1, 14, 8, 0}; /* CTL_KERN, KERN_PROC, KERN_PROC_PROC */
  size_t tam;
  pid_t yo = getpid(), pid = -1;
  if (sysctl(mib, 4, 0, &tam, 0, 0))
    return -1;
  uint8_t *buf = malloc(tam);
  if (!buf || sysctl(mib, 4, buf, &tam, 0, 0)) {
    free(buf);
    return -1;
  }
  for (uint8_t *p = buf; p < buf + tam;) {
    int ki_structsize = *(int *)p;
    pid_t ki_pid = *(pid_t *)&p[72];
    const char *ki_tdname = (const char *)&p[447];
    if (ki_structsize <= 0)
      break;
    if (!strcmp(nombre, ki_tdname) && ki_pid != yo)
      pid = ki_pid;
    p += ki_structsize;
  }
  free(buf);
  return pid;
}

int main(void) {
  syscall(SYS_thr_set_name, -1, SHIMI_NOMBRE);

  /* Una sola copia: si ya hay un shimistore.elf corriendo (p. ej. al enviar una versión nueva), se cierra. */
  for (int i = 0; i < 5; i++) {
    pid_t pid = buscar_pid(SHIMI_NOMBRE);
    if (pid <= 0)
      break;
    shimi_log("Cerrando la copia anterior (pid %d)", pid);
    kill(pid, SIGKILL);
    sleep(1);
  }

  signal(SIGPIPE, SIG_IGN);
  shimi_log("shimiStore %s arrancando (pid %d)", SHIMI_VERSION, getpid());
  shimi_instalar_icono();

  volatile int seguir = 1;
  shimi_aviso("shimiStore %s\nPuerto %d", SHIMI_VERSION, SHIMI_PUERTO);
  int r = shimi_servidor(SHIMI_PUERTO, &seguir);
  if (r)
    shimi_aviso("shimiStore: no pude abrir el puerto %d", SHIMI_PUERTO);
  shimi_log("shimiStore termina");
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
