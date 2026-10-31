# Verifiering 2026-10-31

Testad kod är avsedd som första hårdvaruprovet för Thermia Duo / 901510.
Inget har flashats eller provats mot pumpen i denna miljö.

| Kontroll | Resultat |
| --- | --- |
| BridgeState, C++11 med varningar som fel, AddressSanitizer och UBSan | Godkänd; LeakSanitizer avstängd eftersom processinformation inte är tillgänglig i körmiljön |
| Handshake, register 0, byteordning, timeout, sena svar, felaktiga ramar | Godkänd |
| 1:1-mappning och reserverat Thermia-område 0x0000–0x00FF utan bryggdata | Godkänd |
| Giltighet, verkligt nollvärde, registercykel, timeromslag och åldersmättnad | Godkänd |
| AVR64DD28, PlatformIO 6.2.0, låsta beroenden | Godkänd: 15 948 byte flash, 2 276 byte statisk RAM (registerkarta version 2) |
| AVR64DD28 med den lokalt ändrade ThermiaWire-versionen | Godkänd med separat lokal byggkonfiguration |
| AVR64DD28 med publicerad ThermiaWire-commit `7b5d408` | Godkänd med projektets ordinarie låsta byggkonfiguration |
| ESPHome 2026.10.1 konfigurationsvalidering | Godkänd med tillfälliga lokala hemligheter |
| ESPHome C++-generering | Godkänd |
| C++20-kompilering och beteendetest av den faktiska YAML-lambdan | Godkänd |
| Fullständig ESP32-firmware | Inte verifierad: ESP-IDF:s komponenthanterare stoppades i miljön av `psutil.NoSuchProcess` när processinformation lästes |
| Fysiskt prov med pump, buss och ESP32 | Återstår |

ThermiaWire-ändringen är publicerad på grenen `codex/thermia-nack-visibility`.
`platformio.ini` låser beroendet till commit
`7b5d408a7b0f3550f4e47b25c36deea48d0fce7b`.
Ändringarna granskas i separata draft-PR:er för de två projekten.
ThermiaWire: https://github.com/mtornblad/ThermiaWire/pull/1

Den faktiska EXT-kopplingen, signalnivåerna och vilket ESP32-kort som används
behöver bekräftas. ESPHome-exemplet antar endast ett generiskt ESP32 DevKit.
Registerbetydelser och temperaturskala är inte verifierade. Styrning genom
skrivkommandon är inte implementerad.
