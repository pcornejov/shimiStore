# shimiStore

Tienda de juegos que corre en la PS5, sin PC: un ícono en el inicio abre la tienda, se elige un juego y la consola
lo baja e instala sola. También se puede usar desde el celular en la red local.

## Estructura

| Carpeta | Qué hay | Etapa |
|---|---|---|
| `factibilidad/` | Pruebas con HTTP simple (Python): ¿se llega al archivo sin navegador? | 1 |
| `payload/` | `shimistore.elf` en C con el ps5-payload-sdk: servidor web, descargador, ícono `SHMI00001` | 2, 3, 5 |
| `web/` | Interfaz pensada para el control (HTML/JS sin build, embebida en el payload) | 4 |
| `nube/` | Worker de Cloudflare privado: cron que arma y sirve el catálogo | 4 |
| `catalogo/` | Esquema del catálogo (contrato entre la nube y el payload) | 4 |
| `herramientas/` | Enviar el payload al elfldr (puerto 9021) y ver logs | 2 |

## Decisiones

- **Payload:** C con el [ps5-payload-sdk](https://github.com/ps5-payload-dev/sdk) de John Törnblom.
- **Catálogo:** Worker de Cloudflare privado, con token. Nunca se publica con los links en un lugar público.
- **Licencia:** GPL-3.0, porque se reutiliza código de
  [Payload Manager](https://github.com/itsPLK/ps5-payload-manager).

## Etapa 1

```sh
pip install requests cryptography
python factibilidad/resolver.py hosts
python factibilidad/resolver.py resolver bloodrayne
python factibilidad/resolver.py probar '<link directo>'
```

Los resultados están en [`factibilidad/NOTAS.md`](factibilidad/NOTAS.md).

## Etapa 2: payload mínimo

`shimistore.elf` (versión 0.1.0) hace tres cosas:
- cierra una copia anterior;
- instala el ícono `SHMI00001` en el inicio, que abre `http://127.0.0.1:8090/` a pantalla completa;
- levanta un servidor web en el puerto 8090 que dice "Hola".

Rutas del servidor:

| Ruta | Respuesta |
|---|---|
| `/` | Página "Hola desde shimiStore" |
| `/api/estado` | Versión, segundos encendido y pid, en JSON |
| `/icono.png` | El ícono |
| `/api/apagar` | Cierra el payload |

El log queda en `/data/shimistore/log.txt`.

```sh
cd payload && make             # necesita el ps5-payload-sdk en /opt/ps5-payload-sdk
make enviar PS5=192.168.68.114 # por el elfldr (puerto 9021)
```

**Sin PC:** sube `shimistore.elf` desde el celular con la interfaz web de Payload Manager
(`http://192.168.68.114:8084`) y córrelo desde ahí.

Para cambiar el ícono, edita y corre `herramientas/icono.py`.
