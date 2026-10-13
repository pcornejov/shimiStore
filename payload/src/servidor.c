/* Servidor HTTP mínimo: atiende una conexión a la vez, lo justo para la interfaz y la API.
 * Copyright (C) 2026 Pablo Cornejo. GPL-3.0-or-later.
 */
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "shimi.h"

INCASSET(index_html, "assets/index.html");
INCASSET(icono_png, "assets/icon0.png");

static time_t inicio;

static void responder(int c, int estado, const char *tipo, const void *cuerpo, size_t tam) {
  char cab[256];
  int n = snprintf(cab, sizeof(cab),
                   "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
                   "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
                   estado, estado == 200 ? "OK" : "Not Found", tipo, tam);
  send(c, cab, n, 0);
  for (size_t enviado = 0; enviado < tam;) {
    ssize_t r = send(c, (const char *)cuerpo + enviado, tam - enviado, 0);
    if (r <= 0)
      break;
    enviado += r;
  }
}

/* Atiende una petición; devuelve 0 si se pidió apagar el payload. */
static int atender(int c) {
  char req[2048];
  ssize_t n = recv(c, req, sizeof(req) - 1, 0);
  if (n <= 0)
    return 1;
  req[n] = 0;
  char metodo[8], ruta[256];
  if (sscanf(req, "%7s %255s", metodo, ruta) != 2)
    return 1;

  if (!strcmp(ruta, "/") || !strcmp(ruta, "/index.html")) {
    responder(c, 200, "text/html; charset=utf-8", index_html, index_html_size);
  } else if (!strcmp(ruta, "/icono.png")) {
    responder(c, 200, "image/png", icono_png, icono_png_size);
  } else if (!strcmp(ruta, "/api/estado")) {
    char json[160];
    int len = snprintf(json, sizeof(json), "{\"version\":\"%s\",\"segundos\":%ld,\"pid\":%d}", SHIMI_VERSION,
                       (long)(time(NULL) - inicio), getpid());
    responder(c, 200, "application/json", json, len);
  } else if (!strcmp(ruta, "/api/apagar")) {
    const char *txt = "shimiStore se apaga\n";
    responder(c, 200, "text/plain; charset=utf-8", txt, strlen(txt));
    return 0;
  } else {
    responder(c, 404, "text/plain", "no existe\n", 10);
  }
  return 1;
}

int shimi_servidor(int puerto, volatile int *seguir) {
  inicio = time(NULL);
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    shimi_log("socket: errno %d", errno);
    return -1;
  }
  int uno = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof(uno));
  struct sockaddr_in dir = {.sin_family = AF_INET, .sin_port = htons(puerto), .sin_addr.s_addr = htonl(INADDR_ANY)};
  if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) || listen(s, 16)) {
    shimi_log("No pude abrir el puerto %d: errno %d", puerto, errno);
    close(s);
    return -1;
  }

  while (*seguir) {
    struct pollfd p = {.fd = s, .events = POLLIN};
    if (poll(&p, 1, 500) <= 0)
      continue;
    int c = accept(s, NULL, NULL);
    if (c < 0)
      continue;
    struct timeval espera = {.tv_sec = 5};  /* un cliente lento no traba al resto */
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &espera, sizeof(espera));
    setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, &espera, sizeof(espera));
    if (!atender(c))
      *seguir = 0;
    close(c);
  }
  close(s);
  return 0;
}
