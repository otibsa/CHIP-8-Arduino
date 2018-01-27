/*
 * SpiRAM.cpp - Library for driving a 23k256 SPI attached SRAM chip
 *
 * Phil Stewart, 18/10/2009
 * 
 * Copyright (c) 2009, Phil Stewart
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *  
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *             
 */
/*
 * Updated to Arduino 1.0.5 and newer version of SPI 
 * "2010 by Cristian Maglie"
 * by Fred Jan Kraan, fjkraan@xs4all.nl, 2014-02-28
 */ 
     
//#include <Arduino.h>
#include <SPI.h>
#include <SpiRAM.h>

// Constructor

SpiRAM::SpiRAM(byte clockDiv, byte ssPin)
{
  SPI.begin();
  // Ensure the RAM chip is disabled in the first instance
  disable();

  // Set the spi mode using the requested clock speed
//  SPI.mode(clock);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  _ssPin = ssPin;
  
  // Set the RAM operarion mode flag according to the chip default
  _current_mode = BYTE_MODE;
}

// Enable and disable helper functions

void SpiRAM::enable()
{
  digitalWrite(_ssPin, LOW);
}

void SpiRAM::disable()
{
  digitalWrite(_ssPin, HIGH);
}

// Byte transfer functions

char SpiRAM::read_byte(int address)
{
  char read_byte;

  // Set byte mode
  _set_mode(BYTE_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(READ);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  read_byte = SPI.transfer(0xFF);
  disable();
  
  return read_byte;
}

char SpiRAM::write_byte(int address, char data_byte)
{
  // Set byte mode
  _set_mode(BYTE_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(WRITE);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  SPI.transfer(data_byte);
  disable();

  return data_byte;
}

// Page transfer functions. Bound to current page. Passing the boundary 
//  will wrap to the beginning
void SpiRAM::read_page(int address, char *buffer)
{
  int i;

  // Set byte mode
  _set_mode(PAGE_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(READ);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  for (i = 0; i < 32; i++) {
    buffer[i] = SPI.transfer(0xFF);
  }    
  disable();
}

void SpiRAM::write_page(int address, char *buffer)
{
  int i;

  // Set byte mode
  _set_mode(PAGE_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(WRITE);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  for (i = 0; i < 32; i++) {
    SPI.transfer(buffer[i]);
  }    
  disable();
}

// Stream transfer functions. Ignores page boundaries.
void SpiRAM::read_stream(int address, char *buffer, int length)
{
  int i;

  // Set byte mode
  _set_mode(STREAM_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(READ);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  for (i = 0; i < length; i++) {
    buffer[i] = SPI.transfer(0xFF);
  }    
  disable();
}

void SpiRAM::write_stream(int address, char *buffer, int length)
{
  int i;

  // Set byte mode
  _set_mode(STREAM_MODE);
  
  // Write address, read data
  enable();
  SPI.transfer(WRITE);
  SPI.transfer((char)(address >> 8));
  SPI.transfer((char)address);
  for (i = 0; i < length; i++) {
    SPI.transfer(buffer[i]);
  }    
  disable();
}


// Mode handling
void SpiRAM::_set_mode(char mode)
{
  if (mode != _current_mode)
  {
    enable();
    SPI.transfer(WRSR);
    SPI.transfer(mode);
    disable();
    _current_mode = mode;
  }
}

// Preinstantiate SpiRAM object;
//SpiRAM SpiRam = SpiRAM(RAMCLK1M, 10);
