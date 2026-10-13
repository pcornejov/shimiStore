"""Manda un payload .elf al elfldr de la PS5 (puerto 9021) y, si es shimistore, espera a que responda.

    python herramientas/enviar.py 192.168.68.114 payload/shimistore.elf
"""

import socket
import sys
import time
import urllib.request

ip, elf = sys.argv[1], sys.argv[2]
datos = open(elf, "rb").read()
t0 = time.time()
with socket.create_connection((ip, 9021), timeout=10) as s:
    s.sendall(datos)
print(f"Enviado {elf} ({len(datos) // 1024} KB) a {ip}:9021")

if "shimistore" in elf:
    for _ in range(30):
        try:
            with urllib.request.urlopen(f"http://{ip}:8090/api/estado", timeout=2) as r:
                print(f"Responde en {time.time() - t0:.1f} s: {r.read().decode()}")
                break
        except OSError:
            time.sleep(1)
    else:
        sys.exit("No respondió en 30 s: revisa /data/shimistore/log.txt en la consola")
