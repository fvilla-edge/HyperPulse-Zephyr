# Myriota HyperPulse AT Modem Application

## Overview

The AT Modem application can be used to emulate a stand-alone NTN modem.
The application accepts both the modem-specific AT commands and proprietary AT commands.

> [!IMPORTANT]
> This sample application is for `reference` use only they are not production ready.


- The AT commands are documented in the following guides:

    - Modem-specific AT commands - [nRF91x1 AT Commands Reference Guide](https://docs.nordicsemi.com/bundle/ref_at_commands_nrf91x1/page/REF/at_commands/intro_nrf91x1.html)

    - Nordic Proprietary AT commands

    - Myriota Proprietary AT commands

## Configuration

The AT interface UART can be configured in the devicetree using the `modem,at-host-uart` property.
For example:

```
   / {
      chosen {
         modem,at-host-uart = &uart0;
      };
   };
```

> [!IMPORTANT]  
> All AT commands sent to the HyperPulse AT Host **must** end with a Carriage Return (`<CR>`).  
> A Line Feed (`<LF>`) may optionally follow the Carriage Return.

## Usage

Only one command can be processed at any time, and no data must be sent to the
serial port while the command is being processed.

> [!IMPORTANT]
> For the nRF9151 DK, this application will provide two COM ports, where one will
> be used for application logging and the other as the AT command interface.

> [!IMPORTANT]
> For the HyperPulse Developer Kit (Circuit Dojo), the USB connection on the board
> will provide the AT command interface. To view the logging output
> use the following external pins, P0.23 (UART_RX) and P0.24 (UART_TX).
