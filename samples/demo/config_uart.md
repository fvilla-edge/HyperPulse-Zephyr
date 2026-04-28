# Configuracion UART1 con FTDI (nRF9151 + TF-M)

Este documento deja registrada la configuracion que funciono para recibir datos
por UART desde un FTDI en el sample `samples/demo`, evitando repetir la etapa
de prueba y error.

## Objetivo

Usar `uart1` como puerto de prueba para recibir datos externos (FTDI), dejando
`uart0` para consola/shell.

## Problema tipico

En nRF91 con TF-M, `uart1` puede quedar en conflicto con configuraciones de
seguridad/log del lado secure. Cuando esto pasa, suele verse:

- no llega nada por RX, o
- llegan bytes basura (por ejemplo `0x80` y `0x00`).

## Configuracion que debe quedar en el proyecto

### 1) `prj.conf`

Archivo:
- `samples/demo/prj.conf`

Asegurar:
- `CONFIG_TFM_SECURE_UART1=n`

Esto libera `uart1` para la aplicacion non-secure.

### 2) Overlays de board

Archivos:
- `samples/demo/boards/nrf9151dk_nrf9151_ns.overlay`
- `samples/demo/boards/myriota_hyperpulse_dk_nrf9151_circuitdojo_ns.overlay`

La configuracion valida para `uart1` es:

- `status = "okay";`
- `current-speed = <115200>;`
- `pinctrl-0 = <&uart1_default>;`
- `pinctrl-1 = <&uart1_sleep>;`
- `pinctrl-names = "default", "sleep";`
- `/delete-property/ hw-flow-control;`

Y mantener:
- `zephyr,console = &uart0;`
- `zephyr,shell-uart = &uart0;`

## Comportamiento del firmware en modo test

El modo test UART del demo:

- inicia antes del flujo de GNSS,
- usa `DEVICE_DT_GET(DT_NODELABEL(uart1))`,
- lee con `uart_poll_in(...)`,
- imprime cada byte recibido en HEX + ASCII.

Ejemplo esperado:

- `RX: 0x61 'a'`
- `RX: 0x73 's'`
- `RX: 0x64 'd'`

## Cableado FTDI (checklist)

- FTDI TX -> RX de `uart1` en el nRF
- FTDI RX -> TX de `uart1` en el nRF
- GND -> GND (obligatorio)
- Niveles 3.3V TTL (no RS232, no 5V)

## Configuracion de terminal (PuTTY)

- Baudrate: `115200`
- Data bits: `8`
- Parity: `None`
- Stop bits: `1`
- Flow control: `None`

## Build/flash recomendado

Siempre que cambies algo de TF-M/UART:

1. Hacer **pristine build** (obligatorio).
2. Flashear imagen completa.
3. Enviar una cadena conocida (por ejemplo `asdfgh123`).
4. Verificar en logs que los bytes recibidos coinciden.

## Referencia rapida de diagnostico

Si vuelve a aparecer basura:

1. Confirmar que `CONFIG_TFM_SECURE_UART1=n` sigue presente.
2. Confirmar que overlays siguen con pinctrl default de `uart1` y sin
   `hw-flow-control`.
3. Repetir pristine build.
4. Verificar cableado TX/RX cruzado y GND comun.
5. Revisar que el FTDI este en 3.3V TTL y no en otro modo.
