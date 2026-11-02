# Thermia Duo 901510 – XIAO ESP32-C6 och AVR64DD28

**Schema för Adafruit ISO1540 #4903, revision B, 2026-11-02.**

Gäller om den befintliga modulen är samma Adafruit-kort som i den länkade
Electrokit-produkten. Thermias fysiska EXT-stift och AVR-adaptern återstår att
kontrollera innan lödning.

![Kopplingsschema](wiring-xiao-c6.svg)

Förslaget ersätter den tidigare RS485-kopplingen med direkt UART på samma
labbkort. ThermiaWire kör fortfarande som I²C-slav, pumpen är master.

## Matning och domäner

- Lokal sida: USB-C till XIAO från separat matning. XIAO:s 3V3 förs till AVR VDD,
  AVR VDDIO2 och ISO1540-modulens **sida 2 / VCC**. Modulen stöder 3,3 V.
  Kontrollera den sammanlagda strömbudgeten för XIAO:s regulator.
- Båda AVR-matningspinnarna kopplas fysiskt till 3,3 V, även om MVIO redan är
  aktiverat. Ingen säkringsändring utförs av projektets vanliga byggning.
  Innan ombyggnad kontrolleras även att BOD-nivån är lämplig för 3,3 V.
- Pumpsida: isolatorns pumpsida får sin matning och jordreferens från den
  verifierade pumpkopplingen. `5V_P` är en logisk nätbeteckning, inte en uppgift
  om ett visst stiftnummer eller tillgänglig effekt på EXT-kontakten.
- **GND_P och GND_L förbinds inte.** Inte heller pumpens 5 V och USB:s 5 V
  förbinds. USB/programmerare ansluts endast på den lokala sidan. Isolatorn
  överför signaler, inte matning; gemensam icke-isolerad matning skulle gå runt
  signalernas galvaniska avskiljning.

## ISO1540-modulens orientering

Adafruits sida 1/sida 2 ska inte väljas enbart efter vilken enhet som är en
mikrokontroller. **Thermia är I²C-controller/master och ansluts till sida 1. AVR
är I²C-target/slav och ansluts till sida 2.** Detta följer Adafruits anvisning.

Håll kortet med texten **ISO154x rättvänd**, enligt Adafruits pinoutbild:

- **Sida 1:** nedre stiftraden eller vänstra STEMMA QT-kontakten → Thermia.
- **Sida 2:** övre stiftraden eller högra STEMMA QT-kontakten → AVR/lokal elektronik.

Anslut efter **VCC/GND/SDA/SCL-märkningen**, inte genom att anta samma fysiska
stiftordning på de två raderna. Nedan används funktionsnamn, inte godtyckliga
stiftnummer.

| ISO1540 sida 1, till Thermia | Koppling | ISO1540 sida 2, till AVR | Koppling |
| --- | --- | --- | --- |
| VCC | Verifierad 5V_P | VCC | Lokal 3V3_L |
| GND | GND_P | GND | GND_L |
| SDA | Thermia SDA | SDA | AVR PA2 |
| SCL | Thermia SCL | SCL | AVR PA3 |

Modulens två gröna lysdioder visar att respektive sida är matad. De verifierar
inte att kommunikation eller fysisk jordseparation är korrekt.

## Nätlista

| Nät | Anslutningar |
| --- | --- |
| `3V3_L` | XIAO 3V3, AVR VDD, AVR VDDIO2, ISO1540 sida 2 VCC |
| `GND_L` | XIAO GND, alla AVR GND, ISO1540 sida 2 GND, lokala avkopplingskondensatorer |
| `SDA_L` | AVR PA2, ISO1540 sida 2 SDA |
| `SCL_L` | AVR PA3, ISO1540 sida 2 SCL |
| `UART_AVR_TX` | AVR PC0/TX → XIAO D7/GPIO17/RX |
| `UART_XIAO_TX` | XIAO D6/GPIO16/TX → AVR PC1/RX |
| `UPDI` | AVR UPDI, separat programmeringskontakt med lokal jord och 3,3 V-referens |
| `5V_P`, `GND_P`, `SDA_P`, `SCL_P` | Endast pumpen och isolatorns pumpsida; fysisk EXT-stiftordning återstår |

PC3 används inte. Modbus RTU körs med adress 42, 9600 baud, 8N1.

## Passiva komponenter och isolator

- C1: 100 nF mellan AVR VDD och GND_L, nära matningspinnen.
- C2: 100 nF mellan AVR VDDIO2 och GND_L, nära matningspinnen.
- C5: 4,7 µF mellan 3V3_L och GND_L vid AVR/modul, som lokal bulkavkoppling.
- **Inga extra I²C-pullups monteras som utgångspunkt.** Adafruit #4903 har redan
  10 kΩ från SDA och SCL till respektive sidas VCC, alltså fyra pullupmotstånd.
  Effektiva värden påverkas av eventuell befintlig pullup i pumpen eller på
  AVR-adaptern. Kontrollera uppmätt stigtid och bussfrekvens innan något ändras.
- Adafruits Eagle-schema visar redan **100 nF avkoppling per isolatorsida**.
  De tidigare föreslagna extra C3/C4 vid isolatorn utgår för denna breakout.
  Våra C1/C2 avser AVR, inte de likadant namngivna komponenterna på modulen.
- ISO1540 har dubbelriktad SDA **och SCL**, stöd för clock stretching och
  separata matningar. Därför passar 5 V på pumpens sida och 3,3 V lokalt.
- I2C-isolatorns sidor är funktionellt asymmetriska; sida 1 till pumpens
  controller och sida 2 till AVR följer tillverkarens monteringsanvisning.

Ritningen visar Adafruit-modulen som en enhet, inte en ritning av dess kretskort.
VCC1/GND1/SDA1/SCL1 betecknar sida 1 och motsvarande suffix 2 sida 2.
Om den faktiska modulen avviker från Adafruit #4903 behöver detta kontrolleras
mot dess eget schema innan anslutning.

## Kvar innan lödning

Jämför det befintliga kortet med Adafruit #4903 och bekräfta AVR-kortets verkliga
kapsling/adapter och den fungerande EXT-kopplingens stift. Därefter kan schemat
kompletteras med fysiska stiftnummer på dessa två anslutningar. Layout på labbkortet behöver hålla pump- och lokaljord åtskilda;
schemat i sig fastställer inte en viss säkerhets-/isolationsklass för bygget.

## Primärkällor

- [Seeed XIAO ESP32-C6: D6=GPIO16 TX, D7=GPIO17 RX](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [Microchip AVR64DD32/28 datablad: matning, MVIO och portmultiplexering](https://ww1.microchip.com/downloads/en/DeviceDoc/AVR64DD32-28-Prelim-DataSheet-DS40002315B.pdf)
- [TI ISO1540: separata matningar och dubbelriktad SCL/clock stretching](https://www.ti.com/product/ISO1540)

- [Adafruit #4903 pinout: sida 1/sida 2, matning och 10 kΩ-pullups](https://learn.adafruit.com/adafruit-iso1540-bidirectional-i2c-isolator/pinouts)
- [Adafruits originalschema (Eagle): komponentvärden och nät](https://github.com/adafruit/Adafruit-ISO1540-PCB)
- [Länkad Electrokit-produkt: Adafruit 4903](https://www.electrokit.com/adafruit-iso1540-bidirectional-i2c-isolator)
