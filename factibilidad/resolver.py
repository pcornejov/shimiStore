"""Etapa 1 de shimiStore: ¿se puede llegar al archivo de un juego de Pippo solo con HTTP, sin navegador?

Lee el catálogo de Pippo, descifra los links y resuelve filekeeper hasta el link directo del archivo. Es lo que
después hará el payload en la PS5, así que aquí no se usa nada que la consola no pueda hacer: ni navegador ni
JavaScript, solo GET y POST.

    python factibilidad/resolver.py hosts               # cuántos juegos hay en cada host
    python factibilidad/resolver.py resolver <título>   # link directo de la variante principal en filekeeper
    python factibilidad/resolver.py probar <link>       # Range, tamaño y velocidad de un link directo

Flujo de filekeeper (XFileSharing, medido el 2026-10-13):
  1. GET https://filekeeper.net/<código> → 302 a /download, con la cookie `file_code`.
  2. La página tiene una cuenta regresiva (data-countdown, 5 s) y un formulario que arma con JavaScript.
  3. POST /download con op=download2, id=<código>, method_free="Free download", down_direct=1
     → 302 a https://tunnelN.dlproxy.uk/download/<token>?sig=<firma>.
  El link directo no pide cookies ni User-Agent y acepta Range.

Fileditch no se resuelve: desde octubre de 2026 su página exige una verificación del navegador con WebAssembly.
"""

import base64
import hashlib
import json
import re
import sys
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse

import requests
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

CATALOG_URL = "https://pippo26442999.github.io/.exFAT/exFAT.json"
PASSWORD = b"pippo"  # la publica el propio sitio
# Link Lock usa salt e IV fijos cuando el link no trae los suyos (como en ps5dl/pippo.py).
LOCK_SALT = bytes([236, 231, 167, 249, 207, 95, 201, 235, 164, 98, 246, 26, 176, 174, 72, 249])
LOCK_IV = bytes([255, 237, 148, 105, 6, 255, 123, 202, 115, 130, 16, 116])
UA = "Mozilla/5.0 (PlayStation; PlayStation 5/6.50) AppleWebKit/605.1.15 (KHTML, like Gecko)"

_keys: dict[tuple[bytes, int], bytes] = {}


def _b64(s: str) -> bytes:
    return base64.b64decode(s + "=" * (-len(s) % 4))


def decrypt(link: str) -> str:
    """URL de destino de un link cifrado de Pippo; un link sin cifrar se devuelve tal cual."""
    if "#" not in link or not any(x in link for x in ("decrypt-public", "link-lock-pippo")):
        return link
    p = json.loads(_b64(link.split("#", 1)[1]))
    if "link-lock-pippo" in link:
        salt, iv, rounds = _b64(p["s"]) if p.get("s") else LOCK_SALT, _b64(p["i"]) if p.get("i") else LOCK_IV, 100_000
    else:
        salt, iv, rounds = _b64(p["s"]) if p.get("s") else bytes(16), _b64(p["i"]) if p.get("i") else bytes(12), 600_000
    if (salt, rounds) not in _keys:
        _keys[salt, rounds] = hashlib.pbkdf2_hmac("sha256", PASSWORD, salt, rounds, 32)
    text = AESGCM(_keys[salt, rounds]).decrypt(iv, _b64(p["e"]), None).decode()
    try:
        return json.loads(text).get("u", text)
    except (ValueError, AttributeError):
        return text


def catalog() -> list[dict]:
    r = requests.get(CATALOG_URL, params={"v": int(time.time())}, timeout=60)
    r.raise_for_status()
    return r.json()


def links(game: dict) -> dict[str, str]:
    """{clave: link descifrado} de todas las variantes de un juego ("filek_url", "backport_filek"…)."""
    out = {}
    for key, value in game.items():
        if key != "image" and isinstance(value, str) and value.startswith("http"):
            try:
                out[key] = decrypt(value)
            except Exception as e:  # pylint: disable=broad-except
                print(f"No pude descifrar {key} de {game['title']}: {e}", file=sys.stderr)
    return out


def resolve_filekeeper(url: str) -> str:
    """Link directo de un archivo de filekeeper, solo con HTTP."""
    s = requests.Session()
    s.headers["User-Agent"] = UA
    code = urlparse(url).path.strip("/").split("/")[0]
    page = s.get(url, timeout=30)
    page.raise_for_status()
    wait = re.search(r'data-countdown="(\d+)"', page.text)
    if 'data-has-captcha="true"' in page.text:
        raise RuntimeError("filekeeper pide captcha para este archivo")
    time.sleep(int(wait.group(1)) + 1 if wait else 6)
    r = s.post(page.url, allow_redirects=False, timeout=30, headers={"Referer": page.url},
               data={"op": "download2", "id": code, "rand": "", "referer": "", "method_free": "Free download",
                     "down_direct": "1"})
    location = r.headers.get("location", "")
    if r.status_code not in (301, 302, 303, 307) or not location.startswith("http"):
        raise RuntimeError(f"filekeeper respondió {r.status_code} sin redirección al archivo")
    return location


def probe(url: str, seconds: int = 15, connections: int = 4) -> dict:
    """Tamaño, nombre, si acepta Range y velocidad (una conexión y varias) de un link directo, sin cookies."""
    head = requests.head(url, timeout=30, allow_redirects=True)
    size = int(head.headers.get("content-length", 0))
    name = re.search(r'filename="?([^";]+)', head.headers.get("content-disposition", ""))
    r = requests.get(url, headers={"Range": "bytes=0-1023"}, timeout=30)
    info = {"status": head.status_code, "size": size, "name": name.group(1) if name else None,
            "range": r.status_code == 206 and r.headers.get("content-range", "").startswith("bytes 0-1023/"),
            "server": urlparse(url).hostname}

    def pull(start: int) -> int:
        got, t0 = 0, time.time()
        with requests.get(url, headers={"Range": f"bytes={start}-"}, stream=True, timeout=30) as resp:
            for chunk in resp.iter_content(1 << 16):
                got += len(chunk)
                if time.time() - t0 > seconds:
                    break
        return got

    info["mbs_1"] = round(pull(0) / seconds / 1e6, 1)
    step = size // connections if size else 0
    with ThreadPoolExecutor(connections) as pool:
        total = sum(pool.map(pull, [i * step for i in range(connections)]))
    info[f"mbs_{connections}"] = round(total / seconds / 1e6, 1)
    return info


def main() -> None:
    cmd, args = (sys.argv[1], sys.argv[2:]) if len(sys.argv) > 1 else ("hosts", [])
    if cmd == "hosts":
        games = catalog()
        per_host: Counter = Counter()
        for g in games:
            per_host.update({urlparse(u).hostname or "?" for k, u in links(g).items()
                             if not k.startswith(("dlc", "fix"))})
        print(f"{len(games)} juegos")
        for host, n in per_host.most_common():
            print(f"{n:5}  {host}")
    elif cmd == "resolver":
        words = " ".join(args).lower()
        game = next(g for g in catalog() if words in g["title"].lower())
        url = links(game).get("filek_url") or next(u for u in links(game).values() if "filekeeper" in u)
        print(f"{game['title']} · {' · '.join(game.get('tags') or [])}\n{url}")
        print(resolve_filekeeper(url))
    elif cmd == "probar":
        print(json.dumps(probe(args[0]), indent=2, ensure_ascii=False))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
