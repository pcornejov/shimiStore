# shimiStore

Responde siempre en español. Proyecto personal.

Tienda de juegos que corre **solo en la PS5**, sin PC en el flujo:
- un ícono en el inicio abre la tienda a pantalla completa, navegable con el control;
- desde ahí se busca, se ve la ficha y se descarga;
- la consola baja el juego y queda instalado en el inicio;
- la misma tienda se usa desde el celular en la red local.

## La PS5

- Firmware **13.20**, liberada con WebKit Autoloader v0.6.1. IP `192.168.68.114`.
- Payload Manager (itsPLK, web en el puerto 8084) carga al arrancar, desde `/data/pldmgr/autoload.txt`:
  - nanodns (bloquea PSN);
  - kstuff-lite;
  - ShadowMountPlus 1.7 (monta `.exfat`/`.ffpkg` de `/data/homebrew` y los agrega al inicio);
  - ps5upload 7.0.12.
- Hay un elfldr en el puerto 9021 mientras ps5upload corre.
- **El reposo mata todos los payloads**: después hay que correr el jailbreak a mano.
- Preferencias:
  - la versión más nueva del juego; entre equivalentes, la que mueve menos datos;
  - voces en español latino (de España solo si no hay latino).
- Los payloads pueden colgar la consola: probar en pasos chicos.

## Decisiones tomadas

- **Payload:** C con el ps5-payload-sdk de John Törnblom. Instalarlo desde
  `https://github.com/ps5-payload-dev/sdk/releases/latest/download/ps5-payload-sdk.zip`, descomprimido en `/opt`.
  Necesita clang-18 y lld-18.
- **Catálogo:** un Worker de Cloudflare **privado**, con token. Nunca se publican los links descifrados en un sitio
  público.
- **Licencia:** GPL-3.0. Reutilizamos código de Payload Manager (github.com/itsPLK/ps5-payload-manager).
- **Repo:** privado.

## Referencia: ps5download (repo `pcornejov/ps5download`; leer, no modificar)

Hace todo esto desde el PC con Python y Playwright. Lo útil está en:

| Archivo | Qué hay |
|---|---|
| `ps5dl/pippo.py` | Catálogo de Pippo y descifrado de sus links |
| `ps5dl/hosts/*.py`, `hosts.toml` | Cómo se resuelve cada host |
| `ps5dl/firmware.py` | Firmware que pide cada juego (PROSPEROPatches; necesita navegador por Turnstile) |
| `ps5dl/consola.py`, `ps5dl/ps5upload.py` | Registro de apps, ShadowMount+ y borrado |
| `worker/` | Su Worker de Cloudflare con cron |

## Etapas (en orden; no avanzar sin confirmar la anterior)

1. ✅ **Factibilidad sin la PS5.** Ver `factibilidad/NOTAS.md`.
   - Filekeeper se resuelve solo con HTTP (GET, 5 s de espera, POST `op=download2`). Cubre ~562 de 813 juegos.
   - El link directo admite `Range`, no pide cookies ni depende de la IP, y también baja por HTTP sin TLS.
   - Fileditch ya no sirve: exige una verificación del navegador con WebAssembly.
   - La variante principal es `.exfat`; `dump` y `fpkg` vienen en `.7z`.
   - **Pendiente:** cuánto dura el link (seguía vivo a los 7 min).
2. 🔄 **Payload mínimo** (`payload/`): `shimistore.elf` 0.1.0, compilado y servidor probado en Linux.
   **Falta probarlo en la PS5:**
   - correr `python3 herramientas/enviar.py 192.168.68.114 payload/shimistore.elf`, o subirlo con Payload Manager;
   - ver los dos avisos en pantalla;
   - abrir `http://192.168.68.114:8090`;
   - confirmar que el ícono **shimiStore** (`SHMI00001`) aparece en el inicio y abre a pantalla completa;
   - reenviarlo y confirmar que cierra la copia anterior.
3. **Primera descarga real:** un `.exfat` chico de filekeeper a `/data/homebrew`, montado por ShadowMount+ y
   visible en el inicio. El HTTPS de la PS5 puede ir con libSceHttp2/libSceSsl (hay un ejemplo `http2_get` en el
   SDK) o con mbedTLS; Payload Manager usa libcurl + mbedTLS.
4. **Catálogo en la nube** (Worker) e interfaz de tienda para el control:
   - pantallas: Descubrir, Buscar (filtro "abre en mi firmware"), Ficha, Descargas y Biblioteca;
   - el catálogo trae precalculados el firmware, la nota de Metacritic, el tamaño y los hosts sin captcha.
5. **Reanudación tras el reposo**, hosts asistidos por el navegador de la PS5 y prueba automática de que el juego abre.
   - Payload Manager reinicia su servidor al recibir `SIGCONT` al volver del reposo: revisar si aplica aquí.

## Cómo se trabaja

- `cd payload && make` compila (~3 s).
- `make enviar PS5=192.168.68.114` lo manda al puerto 9021.
- Log en la consola: `/data/shimistore/log.txt`.
