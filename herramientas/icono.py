"""Genera payload/assets/icon0.png: el ícono de shimiStore en el inicio de la PS5 (512×512)."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

SIZE = 512
FONT = "/usr/share/fonts/opentype/inter/InterDisplay-Bold.otf"
OUT = Path(__file__).resolve().parent.parent / "payload/assets/icon0.png"

img = Image.new("RGB", (SIZE, SIZE))
draw = ImageDraw.Draw(img)
for y in range(SIZE):  # degradado vertical: azul noche → violeta
    t = y / (SIZE - 1)
    draw.line([(0, y), (SIZE, y)], fill=(int(18 + 70 * t), int(24 + 10 * t), int(60 + 90 * t)))
# bolsa de compras simplificada
draw.rounded_rectangle([156, 170, 356, 360], radius=28, fill=(255, 196, 61))
draw.arc([206, 110, 306, 230], start=180, end=360, fill=(255, 196, 61), width=18)
draw.text((256, 265), "S", font=ImageFont.truetype(FONT, 130), fill=(28, 30, 70), anchor="mm")
draw.text((256, 432), "shimiStore", font=ImageFont.truetype(FONT, 64), fill=(255, 255, 255), anchor="mm")
OUT.parent.mkdir(parents=True, exist_ok=True)
img.save(OUT, optimize=True)
print(OUT, OUT.stat().st_size, "bytes")
