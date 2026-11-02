# Verifiering – uppdaterad 2026-11-02

Testad kod är avsedd som första hårdvaruprovet för Thermia Duo / 901510.
Inget har flashats eller provats mot pumpen i denna miljö.

| Kontroll | Resultat |
| --- | --- |
| BridgeState, C++11 med varningar som fel, AddressSanitizer och UBSan | Godkänd; LeakSanitizer avstängd eftersom processinformation inte är tillgänglig i körmiljön |
| Handshake, register 0, byteordning, timeout, sena svar, felaktiga ramar | Godkänd |
| 1:1-mappning och reserverat Thermia-område 0x0000–0x00FF utan bryggdata | Godkänd |
| Giltighet, verkligt nollvärde, registercykel, timeromslag och åldersmättnad | Godkänd |
| AVR64DD28, direkt UART till XIAO ESP32-C6, 2026-11-02 | Godkänd: 15 944 byte flash, 2 276 byte statisk RAM |
| AVR64DD28, tidigare RS485-version, PlatformIO 6.2.0 | Godkänd 2026-10-31: 15 948 byte flash, 2 276 byte statisk RAM (registerkarta version 2) |
| Tidigare AVR64DD28 med den lokalt ändrade ThermiaWire-versionen | Godkänd med separat lokal byggkonfiguration |
| Tidigare AVR64DD28 med publicerad ThermiaWire-commit `7b5d408` | Godkänd med projektets ordinarie låsta byggkonfiguration |
| ESPHome 2026.10.1 konfigurationsvalidering | Godkänd med tillfälliga lokala hemligheter |
| ESPHome C++-generering | Godkänd även för XIAO ESP32-C6 / direkt UART 2026-11-02 |
| C++20-kompilering och beteendetest av den faktiska YAML-lambdan | Godkänd, omkörd 2026-11-02 |
| Fullständig ESP32-firmware | Inte verifierad: ESP-IDF:s komponenthanterare stoppades i miljön av `psutil.NoSuchProcess` när processinformation lästes |
| Fysiskt prov med pump, buss och ESP32 | Återstår |
| Preliminärt SVG-schema för isolator + AVR + XIAO | XML-validerat och visuellt kontrollerat; faktisk isolator/EXT-pinout återstår |

ThermiaWire-ändringen är publicerad på grenen `codex/thermia-nack-visibility`.
`platformio.ini` låser beroendet till commit
`7b5d408a7b0f3550f4e47b25c36deea48d0fce7b`.
Ändringarna granskas i separata draft-PR:er för de två projekten.
ThermiaWire: https://github.com/mtornblad/ThermiaWire/pull/1

ESP32-kortet är nu fastställt: **Seeed Studio XIAO ESP32-C6**.
Konfigurationen har validerats och C++ genererats med ESPHome 2026.10.1 för
`seeed_xiao_esp32c6` / ESP32C6, TX GPIO16 och RX GPIO17, utan flödesstyrning.
Den faktiska EXT-kopplingen och den befintliga isolatorns modell/signalnivåer
behöver bekräftas innan det preliminära schemat i `WIRING.md` kan färdigställas.
NODE-inställningen är redan bekräftad rätt av användaren.
Registerbetydelser och temperaturskala är inte verifierade. Styrning genom
skrivkommandon är inte implementerad.
