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
