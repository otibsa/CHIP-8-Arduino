
#include <SPI.h>
#include <SpiRAM.h>

#define SS_PIN 10

byte clock = 0;
SpiRAM spiRam(0, SS_PIN);

void setup() {
  
  Serial.begin(115200);     // writing a byte is ~ 86 uS at 115200 Baud
  delay(1000);              // wait for a second
  Serial.println("Versie 1");  
}

void loop() {
  unsigned long address = 0;
  
}
