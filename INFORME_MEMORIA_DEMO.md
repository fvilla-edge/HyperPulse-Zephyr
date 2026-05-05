# Informe de memoria - `sample/demo`

## 1) Especificaciones del chip

La documentacion de Circuit Dojo para nRF9151 Feather, el SoC nRF9151 tiene:

- **1 MB de Flash**
- **256 KB de RAM**
- **TrustZone habilitado** (separacion Secure / Non-Secure)

Referencia: [Circuit Dojo nRF9151 Feather Specs](https://docs.circuitdojo.com/nrf9151-feather/specs.html)

## 2) TrustZone: que es y como impacta

TrustZone divide el micro en dos dominios aislados:

- **Secure world**: corre TF-M y servicios sensibles.
- **Non-secure world**: corre la aplicacion Zephyr.

Por eso se ve dos consumos de memoria separados (secure y non-secure).


## 3) Presupuesto de memoria por cada parte

En `sysbuild` con TrustZone se construyen dos mundos con presupuesto separado:

- **TF-M secure**
  - FLASH total: `32 KB`
  - RAM total: `32 KB`
- **App non-secure (`demo/zephyr.elf`)**
  - FLASH total: `760 KB`
  - RAM total: `211736 B` (~206.8 KiB)

Importante:

- Son presupuestos separados por particiones/regiones.
- Ambos salen del total fisico del chip (1 MB flash, 256 KB RAM).

## 4) Resultado de compilacion (uso actual)

Valores obtenidos del linker (`Memory region Used Size / Region Size`):

- **TF-M secure**
  - FLASH usada: `32092 B / 32768 B` (97.94%)
  - RAM usada: `10404 B / 32768 B` (31.75%)

- **App non-secure (`demo`)**
  - FLASH usada: `208312 B / 760 KB` (26.77%)
  - RAM usada: `145584 B / 211736 B` (68.76%)
  - IDT_LIST: `0 B / 32 KB`

## 5) Memoria libre 

### TF-M secure

- **Te queda FLASH libre en TF-M:** `676 B`
- **Te queda RAM libre en TF-M:** `22364 B` (~21.8 KiB)

### App non-secure (`demo`)

- **Te queda FLASH libre en la app:** `569928 B` (~556.6 KiB)
- **Te queda RAM libre en la app:** `66152 B` (~64.6 KiB)



## 6) Archivos que manejan TrustZone y mapeo de memoria

### Archivos de configuracion (fuente)

- `boards/myriota/hyperpulse_dk/dts/circuitdojo_feather_nrf9151_partition_conf.dtsi`
  - Define el layout base secure/non-secure en flash y SRAM (`slot0`, `slot0_ns`, `sram0_s`, `sram0_ns`, etc.).
- `samples/demo/pm_static.yml`
  - Fija particiones de flash especificas del proyecto (por ejemplo `nonsecure_storage`, `littlefs_storage`, etc.).
- `samples/demo/sysbuild.conf`
  - Configuracion de sysbuild para la compilacion multi-imagen.

### Archivos de salida (verdad final del build)

- `samples/demo/build/partitions.yml`
  - Mapa final resuelto de particiones y direcciones.
- `samples/demo/build/pm.config`
  - Mismo mapeo final en macros `PM_*` para build/codigo.

Regla practica:

- Para entender o cambiar la intencion del layout: editar archivos de configuracion.
- Para verificar como quedo realmente en ese build: mirar `partitions.yml` y `pm.config`.


## 7) Comandos de reportes (`ram_report`, `rom_report`, `tfm_*`)

Desde `~/ncs/v3.0.2/modules/lib/myriota-hyperpulse`:

```bash
source ~/ncs/env/ncs_3.0.2.sh
west build --build-dir samples/demo/build samples/demo --board myriota_hyperpulse_dk/nrf9151/circuitdojo_ns --sysbuild --pristine

# App non-secure (demo)
west build --build-dir samples/demo/build/demo -t ram_report
west build --build-dir samples/demo/build/demo -t rom_report

# TF-M secure
west build --build-dir samples/demo/build/demo -t tfm_ram_report
west build --build-dir samples/demo/build/demo -t tfm_rom_report
```

Que hace cada uno:

- `ram_report`: desglose de consumo RAM de la app non-secure por modulo/simbolo.
- `rom_report`: desglose de consumo Flash (ROM) de la app non-secure.
- `tfm_ram_report`: desglose de consumo RAM de TF-M (secure).
- `tfm_rom_report`: desglose de consumo Flash de TF-M (secure).

