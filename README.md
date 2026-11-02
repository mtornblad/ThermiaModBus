# Thermia 901510 → AVR → XIAO ESP32-C6 → Home Assistant

Brygga för Mattias Thermia Duo med styrkort **901510** och **AVR64DD28**.
AVR-kretsen hanterar Thermias I²C-protokoll med ThermiaWire. ESP32 läser
bryggans Modbus RTU-register och ansluter till Home Assistant med ESPHomes API.

## Status

Det här är en **första version för avläsning och verifiering**, inte färdig
pumpstyrning. Automatisk hämtning av `0x00–0x0F` bygger vidare på området i
ursprungskoden. Registerbetydelse, skala och svarstider måste verifieras mot
just den här pumpen. Kortets ålder bevisar inte protokollrevisionen.

- Ett register i taget efterfrågas. Pumpen är I²C-master, AVR är slav på `0x2E`.
- `0xFE` NACK:as när inget nytt uppdrag finns. När en läsning väntar accepteras
  frågan och AVR lämnar registernumret när pumpen läser. Pumpens svar tolkas som
  `[register, low, high]`, precis som i den ursprungliga firmwareversionen.
- Ingen Modbus-hantering eller seriell loggning görs i I²C-avbrottet.
- Timeout efter 5 sekunder; nästa register efter 250 ms. Värden blir gamla efter
  120 sekunder utan ett nytt giltigt svar. Tiderna är startvärden för provning.
- ESPHome visar 16 råregister. Ett äkta nollvärde skiljs från ett aldrig mottaget
  värde. Giltighet och råvärde läses i samma Modbus-svar.
- **Inga pumpinställningar kan skrivas.** Holdingregister och coils är inte
  konfigurerade. Skrivning kräver verifierad registerkarta, gränser och återläsning.

## Bygg AVR-firmware

Installera PlatformIO och kör från projektmappen:

```sh
pio run -e avr64dd28
```

Beroendena är låsta till versioner/commits och hämtas via HTTPS. Ingen GitHub
SSH-nyckel behövs. `avr64dd28` bygger utan att anta programmerare eller COM-port.

Befintliga uppladdningsalternativ finns kvar:

```sh
pio run -e jtag2updi -t upload --upload-port COM9
pio run -e pymcuprog_upload -t upload --upload-port /dev/ttyAMA1
```

`pymcuprog` måste installeras separat om det alternativet används. Anpassa porten
på kommandoraden. Ingen uppladdning eller ändring av säkringsbitar sker vid vanlig byggning.
Seriell startlogg går på `Serial` med 115200 baud; Modbus använder `Serial1`.

## Anslutningar: XIAO ESP32-C6 och direkt UART

Se [kopplingsschemat](docs/wiring-xiao-c6.svg) och
[matning och isolering](docs/WIRING.md). Schemat är preliminärt tills den
befintliga I²C-isolatorn och Thermias EXT-stift har identifierats.

För den nya konstruktionen föreslås AVR och XIAO på samma lokala 3,3 V-domän.
AVR:s **VDD och VDDIO2 ansluts båda till 3,3 V**. MVIO kan behållas aktiverat med
samma spänning på båda matningspinnarna; det behövs ingen 5 V-domän på AVR-kortet
om isolatorns lokala sida fungerar med 3,3 V. Isolatorns pumpsida arbetar med
pumpens verifierade 5 V-matning och jord. De två jorddomänerna hålls åtskilda.

| Lokal anslutning | Motpart |
| --- | --- |
| AVR PA2 / SDA | Isolatorns lokala SDA |
| AVR PA3 / SCL | Isolatorns lokala SCL |
| AVR PC0 / Serial1 TX | XIAO **D7 / GPIO17 / RX** |
| AVR PC1 / Serial1 RX | XIAO **D6 / GPIO16 / TX** |
| AVR VDD och VDDIO2 | Lokal 3,3 V |
| AVR GND | XIAO GND och isolatorns lokala GND |
| AVR UPDI | Programmerare med 3,3 V-signaler och lokal jordreferens |

Inga RS485-transceivrar eller DE-/RE-signaler används. PC3 är ledig.
Modbus RTU är fortfarande protokollet: **adress 42, 9600 baud, 8N1**.
ArduinoRS485 är kvar som programvaruberoende eftersom ArduinoModbus använder dess
transportklass; den skapas med både DE och /RE satta till `-1`.

Firmware använder standardplaceringarna för TWI och USART1. Portnamnen ovan är
inte fysiska kapselstiftnummer. En tidigare koppling med TWI på MVIO-pinnarna
PC2/PC3 måste dras om till PA2/PA3 för denna version. Koppla inte XIAO till en
AVR-UART som fortfarande drivs med 5 V.

## ESPHome och Home Assistant

1. Kopiera hela `esphome/` till din ESPHome-konfigurationsmapp inklusive `packages/`.
2. Kopiera `secrets.example.yaml` till `secrets.yaml` eller slå ihop nycklarna med
   din befintliga secrets-fil. Sätt Wi-Fi, OTA-lösenord och en unik API-nyckel.
3. Generera API-nyckeln med `openssl rand -base64 32`. Exempeltexten är ingen nyckel.
4. Konfigurationen är anpassad till XIAO ESP32-C6: TX GPIO16/D6, RX GPIO17/D7. Kontrollera att kopplingen följer tabellen ovan.
5. Kör `esphome config thermia-duo.yaml` och `esphome run thermia-duo.yaml`.
6. Lägg till enheten i Home Assistants **ESPHome**-integration och ange API-nyckeln.

Modbus-inställningarna är **adress 42, 9600 baud, 8N1, funktion 04**.
Använd versionen av firmware i den här ändringen tillsammans med YAML-exemplet;
ursprungsversionen hade inte statusorden som YAML nu behöver.

Namnen `Register 00 raw` till `Register 0f raw` är avsiktligt neutrala.
När värden jämförts med displayen kan sensorerna få rätt namn, enhet, teckentolkning
och skala. Nu publiceras råa 16-bitars ord utan enhet; tvåkomplementsvärdet `65526`
kan exempelvis betyda `-10`, men rätt temperaturskala är ännu inte fastställd.

ESPHome läser värde, status och ålder i ett gemensamt sammanhängande registerområde.
Lambda-funktionen kontrollerar status i samma svar och returnerar `NAN` om värdet
saknas eller är gammalt. Om Modbus slutar svara blir råsensorerna ogiltiga efter
30 sekunder utan uppdatering. Om bara pumpkommunikationen upphör löper AVR:s
120-sekunders giltighetstid ut först, plus upp till ett ESPHome-avläsningsintervall.

## Registerkarta (version 2)

Alla adresser är **nollbaserade input registers**, funktion 04. Thermia-register
`n` motsvarar Modbus-adress `n`, med oförändrat råvärde. Hela området
`0x0000–0x00FF` är reserverat för Thermia; bryggans egna register börjar på
`0x0100`. Just nu hämtas endast `0x00–0x0F`. Övriga adresser i Thermia-området är
reserverade och returnerar tills vidare noll, vilket inte är ett uppmätt värde.

Detta är vår Modbus-brygga; pumpen kommunicerar fortfarande över I²C.
Ange det faktiska registernumret i klienten, utan att lägga till 30001/40001.

| Adress | Innehåll |
| --- | --- |
| `0x0000–0x000F` | Thermia-register `0x00–0x0F`, direkt 1:1 |
| `0x0010–0x00FF` | Reserverat för ytterligare Thermia-register, ännu ej avlästa |
| `0x0100` | Identifierare `0x5448` (TH) |
| `0x0101` | Kartversion, `2` |
| `0x0102` | Färska register, bitmask (bit n = register n) |
| `0x0103` | Någonsin mottagna register, bitmask |
| `0x0104` | Antal giltiga registersvar |
| `0x0105` | Antal timeouter |
| `0x0106` | Antal felaktiga/ej stödda mottagna ramar |
| `0x0107` | Antal accepterade `0xFE`-frågor |
| `0x0108` | Väntande register, `0xFFFF` om inget väntar |
| `0x0109–0x011F` | Reserverat, noll |
| `0x0120 + 3×n` | Råvärde för register n |
| `0x0121 + 3×n` | Status: `0` aldrig mottaget, `1` färskt, `2` gammalt |
| `0x0122 + 3×n` | Ålder i sekunder, max `65534`; `65535` = aldrig mottaget |

`0x0200` och uppåt reserveras för eventuella styrregister för **bryggan**.
De är inte implementerade eller skrivbara ännu. När skrivning av pumpinställningar
införs ska även dessa adresser följa Thermia-numreringen 1:1 i holdingregister;
bryggkommandon får inte ta över adresser i Thermia-området. Input- och
holdingregister är separata Modbus-tabeller, så även registertypen måste anges.

Den extra vyn från `0x0120` duplicerar råvärdet tillsammans med status och ålder.
ESPHome-exemplet använder denna vy för att få värde och giltighet i samma svar.
Den vanliga 1:1-vyn finns alltid kvar på Thermias ursprungliga adresser.

`n` är `0–15`. Räknarna är 16-bitars, slår runt vid 65536 och nollställs vid
omstart. Råvärden behåller pumpens bitmönster; Modbus skickar hög byte först som
vanligt, så välj inte byte-swap i ESPHome. Komplettera 1:1-råvyn med giltighetskontroll i automationer:
den saknar egen giltighetsmarkering och behåller senaste värdet vid bortfall.
En till synes rimlig siffra bevisar inte att rätt parameter har identifierats.

## Provning på breadboard

1. Bygg båda firmwarerna, kontrollera den nya 3,3 V-matningen, pinval och isolatorns två matningsdomäner.
2. Prova först ESP32↔AVR. Registren `0x0100/0x0101` ska ge `0x5448/2` även utan pump.
   Råsensorerna ska förbli ogiltiga tills verkliga registersvar kommer.
3. Återanslut pumpkopplingen genom den verifierade isolatorn. Kontrollera att pumpens display och
   ordinarie funktion fortsätter fungera. Se om `I2C accepted queries` och
   `I2C register responses` ökar.
4. Notera alla 16 råvärden samt displayens temperaturer, integral och driftläge
   vid samma tidpunkt. Två eller tre mätserier hjälper att fastställa skalan.
5. NODE-inställningen är redan bekräftad rätt för den befintliga koden.
   Om timeouter räknas upp men svar saknas efter ombyggnaden: kontrollera
   isolatorns matningar, pullups, SDA/SCL och den tidigare fungerande
   handskakningen innan registerområde eller tider ändras.
6. Prova bortfall mellan AVR och ESP32: inga gamla råvärden ska fortsätta som
   giltiga HA-värden efter 30 sekunder. Kontrollera pumpbortfall separat.

För att färdigställa styrningen återstår: verifiera registerkartan och kortrevision,
välja parametrar som ska styras, implementera begränsade skrivkommandon med
återläsning, och dokumentera kopplingen med de faktiska korten före lödning.

## Tester

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -I include test/native/test_bridge.cpp -o /tmp/test-bridge
/tmp/test-bridge
pio run -e avr64dd28
esphome config esphome/thermia-duo.yaml
esphome compile --only-generate esphome/thermia-duo.yaml
python test/test_esphome_lambda.py
```

Native-testet använder samma tillståndsmaskin som firmware och verifierar
handskakning, register noll, byteordning, felaktiga ramar, sena svar, timeout,
registercykel, giltighet, riktiga nollor och timeromslag. Tester ersätter inte ett
prov mot pumpens buss. CI använder tillfälliga testhemligheter för ESPHome.

Den separata lambda-kontrollen kompilerar uttrycket direkt ur YAML och testar
saknade/gamla värden, verklig nolla, korta svar och förskjutning i ett samlat svar.
Det är inte en fullständig ESP32-firmwarekompilering.

## Referenser

- [ThermiaWire-forken](https://github.com/mtornblad/ThermiaWire)
- [Husdata om 901510:s olika generationer](https://husdata.se/thermia-diplomat/)
- [Husdata, anslutning och äldre NODE-inställning](https://husdata.se/docs/heat-pump-installation/60-thermia-diplomat/thermia-diplomat-installation/)
- [ESPHome Modbus controller](https://esphome.io/components/modbus_controller/)
- [ESPHome Modbus-sensorer](https://esphome.io/components/sensor/modbus_controller/)
- [DxCore:s AVR64DD28-pinout](https://github.com/mtornblad/DxCore/blob/77292d36c7c7d1e7ca79bbfbfb61915554229657/megaavr/variants/28pin-ddseries/pins_arduino.h)
