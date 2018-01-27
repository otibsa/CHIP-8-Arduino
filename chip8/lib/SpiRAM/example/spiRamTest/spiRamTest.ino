
#include <SPI.h>
#include <SpiRAM.h>

#define SS_PIN 10

byte clock = 0;
SpiRAM spiRam(0, SS_PIN);

void setup() {
  digitalWrite(A5, HIGH);
  Serial.begin(115200); 
  delay(1000);              // wait for a second
  Serial.println("Version 1");  
}

// The RAM, initially having random contents, will fail the test. If A5 is 
// pulled to ground, the correct value will be written before test, and the
// RAM location written to, will pass the test. Keeping A5 low for the full 
// cycle from 0 to 7FFF will make the test completely happy. Until the next
// next power cycle.
void loop() {
  unsigned int address = 0;
  unsigned int i;
  unsigned int maxRam = 32768;
  byte wValue, rValue;
  for (i=0; i < maxRam; i++) {
    wValue = (byte) (i & 0xFF);
    if (digitalRead(A5) == LOW) {
      spiRam.write_byte(i, wValue);
    }
    rValue = (byte)spiRam.read_byte(i);
    if (wValue != rValue) {
      Serial.print("RAM error at ");
      Serial.print(i, HEX);
      Serial.print(" should be ");
      Serial.print(wValue, HEX);
      Serial.print(" is ");
      Serial.println(rValue, HEX);
    } else {
      Serial.println(i, HEX);
    }
  }
}
