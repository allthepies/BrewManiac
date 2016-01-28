# BrewManiac
an Open ArdBir alternative.

History

2016-01-24  - allthepies
Forked Vito's original and made the following changes:-

* Removal of mash stages and naming simplification.
* Removal of Iodine test and cooling process steps.
* Removal of all fahrenheit code.
* Reorganisation of EEPROM persistent storage mapping to reflect above.
* Addition of compile time capability to change measurement resolution of DS18B20 temp probe from between 10 and 12 bits.

Fixes
In manual mode, flashing countdown time kept on flashing if return to main screen was selected.


Vito's original README comments follow:-

BrewManiac is an ARDUINO sketch that minics Open ArdBir. 
BrewManiac is crated from scratch but its interface and behavior is almost the same as Open ArdBir.
I wanted to add wireless capability to Open ArdBir but ended up learning that writing a new sketch might be simpler.
The main difference between Open ArdBir and BrewManiac is the program structure. 
BrewManiac is event-driven while Open Ardbir is mor like a series of functoins.

Current implementation supports HM10, a BLE transceiver.

Usage:
1.download necessary library, including
EEPROM
OneWire
PID_v1
SoftwareSerial
LiquidCrystal
Wire (for I2C LCD)
LiquidCrystal_I2C (for I2C LCD)

2.downlad the code and open the main sketch file to modify the hardware setting.

(to Be Continue...)
