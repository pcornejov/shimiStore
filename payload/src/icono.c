/* Ícono SHMI00001 en el inicio de la PS5: al abrirlo, la consola carga deeplinkUri (la tienda) a pantalla completa.
 *
 * Adaptado de src/app_installer.c de Payload Manager (itsPLK, GPL-3.0), que a su vez se basa en ftpsrv de John
 * Törnblom: se escriben /user/app/<id>/sce_sys/{param.json,icon0.png} y se registra la carpeta con
 * sceAppInstUtilAppInstallTitleDir.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <ps5/kernel.h>

#include "shimi.h"

INCASSET(param_json, "assets/param.json");
INCASSET(icon0_png, "assets/icon0.png");

#define APP_DIR "/user/app/" SHIMI_TITLE_ID
#define SCE_SYS APP_DIR "/sce_sys"

int sceAppInstUtilInitialize(void);
int sceAppInstUtilTerminate(void);
int sceAppInstUtilAppInstallAll(void *);

/* 1 si el archivo no existe o no tiene exactamente ese contenido. */
static int distinto(const char *ruta, const uint8_t *datos, size_t tam) {
  struct stat st;
  if (stat(ruta, &st) || (size_t)st.st_size != tam)
    return 1;
  FILE *f = fopen(ruta, "r");
  if (!f)
    return 1;
  uint8_t *buf = malloc(tam);
  int dif = !buf || fread(buf, 1, tam, f) != tam || memcmp(buf, datos, tam);
  free(buf);
  fclose(f);
  return dif;
}

static int escribir(const char *ruta, const uint8_t *datos, size_t tam) {
  FILE *f = fopen(ruta, "w");
  if (!f)
    return -1;
  int ok = fwrite(datos, tam, 1, f) == 1;
  fclose(f);
  return ok ? 0 : -1;
}

static int registrar(void) {
  int (*install_title_dir)(const char *, const char *, void *) = 0;
  uint32_t handle;
  if (!kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle))
    install_title_dir = (void *)kernel_dynlib_resolve(-1, handle, "Wudg3Xe3heE");
  if (install_title_dir)
    return install_title_dir(SHIMI_TITLE_ID, "/user/app/", 0);
  return sceAppInstUtilAppInstallAll(0);
}

int shimi_instalar_icono(void) {
  const char *param = SCE_SYS "/param.json", *icono = SCE_SYS "/icon0.png";
  if (!distinto(param, param_json, param_json_size) && !distinto(icono, icon0_png, icon0_png_size)) {
    shimi_log("Ícono %s ya instalado", SHIMI_TITLE_ID);
    return 0;
  }

  int err = sceAppInstUtilInitialize();
  if (err) {
    shimi_log("sceAppInstUtilInitialize: 0x%08X", err);
    return -1;
  }
  if ((mkdir(APP_DIR, 0755) && errno != EEXIST) || (mkdir(SCE_SYS, 0755) && errno != EEXIST)) {
    shimi_log("No pude crear %s (errno %d)", SCE_SYS, errno);
    err = -1;
  } else if (escribir(param, param_json, param_json_size) || escribir(icono, icon0_png, icon0_png_size)) {
    shimi_log("No pude escribir param.json o icon0.png");
    err = -1;
  } else if ((err = registrar())) {
    shimi_log("Registro del ícono: 0x%08X", err);
  } else {
    shimi_aviso("shimiStore: ícono instalado en el inicio");
  }
  sceAppInstUtilTerminate();
  return err ? -1 : 0;
}
