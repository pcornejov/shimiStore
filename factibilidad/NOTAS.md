# Etapa 1: factibilidad sin la PS5

Medido el 2026-10-13 desde un servidor en la nube (IP de datacenter), solo con HTTP: `requests` y `curl`, sin
navegador. El script es `resolver.py`.

## Catálogo de Pippo (813 juegos)

`exFAT.json` es público y los links se descifran sin navegador: AES-GCM con la clave `pippo`, PBKDF2 de 600 000 o
100 000 iteraciones. Las variantes "decrypt-public" comparten el salt, así que la clave se deriva una sola vez.

Juegos con al menos un link en cada host, contando todas las variantes salvo DLC y fixes:

| Host | Juegos | ¿Solo con HTTP? |
|---|---|---|
| akirabox | 753 | no (verificación) |
| vikingfile | 752 | no (verificación) |
| datanodes | 705 | no (gratis solo deja bajar desde el navegador) |
| **filekeeper** | **562** | **sí** |
| datavaults | 538 | no (reCAPTCHA) |
| fileditch | 48 | **ya no**: desde octubre de 2026 pide una verificación del navegador con WebAssembly |
| buzzheavier | 47 | no (Cloudflare) |
| link-vault.org | 45 | no (Cloudflare) |

- **Filekeeper cubre 562 juegos (69 %).** En la variante principal (`filek_url`) son 457.
- Fileditch solo aportaría 2 juegos más, así que no vale la pena.
- Hay 43 juegos que solo están en link-vault.

**Formato:**
- La variante principal y los backports son `.exfat` de un solo archivo, que ShadowMount+ monta directo.
- `dump` y `fpkg` vienen en `.7z`, por ejemplo `PPSA16603.7z` de 25 GB. Habría que descomprimirlos: espacio doble.
- Para la tienda se priorizan las variantes `.exfat`.

## Filekeeper: flujo sin navegador

1. `GET https://filekeeper.net/<código>` → 302 a `/download`, con la cookie `file_code`.
2. La página trae `data-countdown="5"` y `data-has-captcha="false"`.
3. Tras la espera: `POST /download` con `op=download2&id=<código>&rand=&referer=&method_free=Free download&down_direct=1`.
4. Respuesta: `302 Location: https://tunnel1.dlproxy.uk/download/<token>?sig=<firma>`.

Tarda unos 8 a 11 s por archivo. Se resolvieron 8 seguidos sin rechazos ni captcha.

## El link directo

| Pregunta | Resultado |
|---|---|
| ¿Acepta `Range`? | **Sí**: `206` con `content-range` correcto, también en mitad del archivo; `accept-ranges: bytes` |
| ¿Pide cookies? | **No**: funciona con `curl` pelado, sin cookies, User-Agent ni Referer |
| ¿Trae tamaño y nombre? | Sí: `content-length` y `content-disposition: filename="PPSA05916.exfat"`, más `etag` |
| ¿Cuánto dura? | Seguía vivo a los 7 min; prueba a las 3 h pendiente |
| ¿Depende de la IP? | **No.** La salida de la nube rota de IP en cada conexión (160.79.106.x) y 12 de 12 rangos dieron 206. El link resuelto ahí también lo entregó una IP de otra red (34.75.111.51, Google Cloud) |
| ¿Responde por HTTP sin TLS? | El archivo sí (`http://tunnel1.dlproxy.uk/…` → 206). filekeeper.net redirige a HTTPS |
| Velocidad | Desde el datacenter: ~3,6 MB/s con 1 conexión y ~120 MB/s con 4. Falta medir en la casa |

## Conclusión provisional

Unos 560 juegos (~69 %) se pueden bajar solo con HTTP. El resto queda para los hosts asistidos de la etapa 5.

Consecuencias para el payload:
- Los resolvedores no necesitan JavaScript: un GET, una espera y un POST sin seguir la redirección.
- El descargador puede usar rangos y varias conexiones, y reanudar con `Range` + `etag`.
- Necesita TLS para filekeeper.net (pocas peticiones chicas). El archivo grande también baja por HTTP sin TLS.
- Como el link no depende de la IP, la nube podría resolverlo por adelantado. Igual conviene que lo resuelva la PS5
  justo antes de bajar, por si vence.
- Dos resoluciones del mismo archivo dieron el mismo `sig` y casi el mismo token: la firma parece ir por archivo.
