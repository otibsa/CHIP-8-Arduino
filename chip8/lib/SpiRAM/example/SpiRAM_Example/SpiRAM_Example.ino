#include <SPI.h>
#include <SpiRAM.h>

#define SS_PIN 10

byte clock = 0;
SpiRAM SpiRam(0, SS_PIN);

void setup()   {                
  Serial.begin(9600);
}

void loop()                    
{
  char data_to_chip[17] = "Testing 90123456";
  char data_from_chip[17] = "                ";
  int i = 0;

  // Write some data to RAM
  SpiRam.write_stream(0, data_to_chip, 16);
  delay(100);

  // Read it back to a different buffer
  SpiRam.read_stream(0, data_from_chip, 16);

  // Write it to the serial port
  for (i = 0; i < 16; i++) {
    Serial.print(data_from_chip[i]);
  }
  Serial.print("\n");
  delay(1000);                  // wait for a second
}
