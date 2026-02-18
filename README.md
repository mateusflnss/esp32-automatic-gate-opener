# ESP32 Automatic Gate Opener with ESP-NOW

Personal IoT project: motorcycle proximity detector opens home gate automatically.

## System Overview

- **Sender** (on motorcycle): detects ignition + proximity → sends secure packets via ESP-NOW
- **Receiver** (at gate): validates rolling code → controls relay to open/close gate
- Features: rolling code security, RSSI-based approaching detection, OTA updates, button debouncing with ring buffer

## Current status (Feb 2026)

- Receiver firmware: OTA via physical button toggle, basic gate control logic
- Sender firmware: periodic pings + command sending
- Not yet fully tested on hardware — OTA on sender still WIP
- Working on: end-to-end testing, video demo, cleanup of duplicated modules

## Hardware

- 2× ESP32 dev boards
- Relay module for gate motor
- Button for OTA on receiver

## Português (pt-BR)
- Projeto pessoal que usa dois módulos esp32 para detectar a aproximação da moto ao portão, fazendo a abertura automatica dele

