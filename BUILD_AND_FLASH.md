# Compilar y flashear el demo

Desde la raíz del repo (`myriota-hyperpulse`).

## Comandos

| Comando | Qué hace |
|--------|----------|
| `make build` | Compila con `west` (sysbuild). Por defecto usa `--pristine` (build limpio, más lento). |
| `make build PRISTINE=0` | Compila **sin** `--pristine` (incremental, más rápido si ya compilaste antes). |
| `make flash` | Sube `samples/demo/build/merged.hex` con `pyocd`. |
| `make erase` | Borra el chip con `pyocd erase --chip` (cuidado: borra todo el flash). |
| `make all` | `build` y luego `flash` en una sola pasada. |

Lo mismo con el script:

```bash
./scripts/build_flash.sh build
./scripts/build_flash.sh build --no-pristine   # igual que PRISTINE=0
./scripts/build_flash.sh flash
./scripts/build_flash.sh erase
./scripts/build_flash.sh all
```

## Opcional

- **Otro target de pyocd:** `PYOCD_TARGET=nrf91 make flash` (por defecto es `nRF91`).
