/*
  EEPROM.h -ported by Paolo Becchi to Esp32 from esp8266 EEPROM
           -Modified by Elochukwu Ifediora <ifedioraelochukwuc@gmail.com>
           -Converted to nvs lbernstone@gmail.com

  Uses a nvs byte array to emulate EEPROM

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "EEPROM.h"

EEPROMClass::EEPROMClass(void)
  : _handle(0)
  , _data(0)
  , _size(0)
  , _dirty(false)
  , _name("eeprom")
  , _user_defined_size(0)
{
}

EEPROMClass::EEPROMClass(uint32_t sector)
// Only for compatiility, no sectors in nvs!
  : _handle(0)
  , _data(0)
  , _size(0)
  , _dirty(false)
  , _name("eeprom")
  , _user_defined_size(0)
{
}

EEPROMClass::EEPROMClass(const char* name, uint32_t user_defined_size)
  : _handle(0)
  , _data(0)
  , _size(0)
  , _dirty(false)
  , _name(name)
  , _user_defined_size(user_defined_size)
{
}

EEPROMClass::~EEPROMClass() {
  end();
}

bool EEPROMClass::begin(size_t size) {
  if (!size) {
      return false;
  }

  if (_data) {
    delete[] _data;
  }

  _data = (uint8_t*) malloc(size);
  if(!_data) {
    return false;
  }
  _size = size;
  return true;
}

void EEPROMClass::end() {
  if (!_size) {
    return;
  }

  commit();
  if (_data) {
    delete[] _data;
  }
  _data = 0;
  _size = 0;

  _handle = 0;
}

uint8_t EEPROMClass::read(int address) {
  if (address < 0 || (size_t)address >= _size) {
    return 0;
  }
  if (!_data) {
    return 0;
  }

  return _data[address];
}

void EEPROMClass::write(int address, uint8_t value) {
  if (address < 0 || (size_t)address >= _size)
    return;
  if (!_data)
    return;

  // Optimise _dirty. Only flagged if data written is different.
  uint8_t* pData = &_data[address];
  if (*pData != value)
  {
    *pData = value;
    _dirty = true;
  }
}

bool EEPROMClass::commit() {
  bool ret = false;
  if (!_size) {
      return false;
  }
  if (!_data) {
      return false;
  }
  if (!_dirty) {
      return true;
  }

  return ret;
}

uint8_t * EEPROMClass::getDataPtr() {
  _dirty = true;
  return &_data[0];
}

/*
   Get EEPROM total size in byte defined by the user
*/
uint16_t EEPROMClass::length ()
{
  return _user_defined_size;
}

/* 
   Convert EEPROM partition into nvs blob
   Call convert before you call begin
*/
uint16_t EEPROMClass::convert (bool clear, const char* EEPROMname, const char* nvsname)
{
  uint16_t result = 0;

 
  return result;
}

/*
   Read 'value' from 'address'
*/
uint8_t EEPROMClass::readByte (int address)
{
  uint8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int8_t EEPROMClass::readChar (int address)
{
  int8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint8_t EEPROMClass::readUChar (int address)
{
  uint8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int16_t EEPROMClass::readShort (int address)
{
  int16_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint16_t EEPROMClass::readUShort (int address)
{
  uint16_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int32_t EEPROMClass::readInt (int address)
{
  int32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint32_t EEPROMClass::readUInt (int address)
{
  uint32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int32_t EEPROMClass::readLong (int address)
{
  int32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint32_t EEPROMClass::readULong (int address)
{
  uint32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int64_t EEPROMClass::readLong64 (int address)
{
  int64_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint64_t EEPROMClass::readULong64 (int address)
{
  uint64_t value = 0;
  return EEPROMClass::readAll (address, value);
}

float_t EEPROMClass::readFloat (int address)
{
  float_t value = 0;
  return EEPROMClass::readAll (address, value);
}

double_t EEPROMClass::readDouble (int address)
{
  double_t value = 0;
  return EEPROMClass::readAll (address, value);
}

bool EEPROMClass::readBool (int address)
{
  int8_t value = 0;
  return EEPROMClass::readAll (address, value) ? 1 : 0;
}

size_t EEPROMClass::readString (int address, char* value, size_t maxLen)
{
  if (!value)
    return 0;

  if (address < 0 || address + maxLen > _size)
    return 0;

  size_t len;
  for (len = 0; len <= _size; len++)
    if (_data[address + len] == 0)
      break;

  if (address + len > _size)
    return 0;

  memcpy((uint8_t*) value, _data + address, len);
  value[len] = 0;
  return len;
}

String EEPROMClass::readString (int address)
{
  if (address < 0 || (size_t)address > _size)
    return String();

  size_t len;
  for (len = 0; len <= _size; len++)
    if (_data[address + len] == 0)
      break;

  if (address + len > _size)
    return String();

  char value[len];
  memcpy((uint8_t*) value, _data + address, len);
  value[len] = 0;
  return String(value);
}

size_t EEPROMClass::readBytes (int address, void* value, size_t maxLen)
{
  if (!value || !maxLen)
    return 0;

  if (address < 0 || address + maxLen > _size)
    return 0;

  memcpy((void*) value, _data + address, maxLen);
  return maxLen;
}

template <class T> T EEPROMClass::readAll (int address, T &value)
{
  if (address < 0 || address + sizeof(T) > _size)
    return value;

  memcpy((uint8_t*) &value, _data + address, sizeof(T));
  return value;
}

/*
   Write 'value' to 'address'
*/
size_t EEPROMClass::writeByte (int address, uint8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeChar (int address, int8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUChar (int address, uint8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeShort (int address, int16_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUShort (int address, uint16_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeInt (int address, int32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUInt (int address, uint32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeLong (int address, int32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeULong (int address, uint32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeLong64 (int address, int64_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeULong64 (int address, uint64_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeFloat (int address, float_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeDouble (int address, double_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeBool (int address, bool value)
{
  int8_t Bool;
  value ? Bool = 1 : Bool = 0;
  return EEPROMClass::writeAll (address, Bool);
}

size_t EEPROMClass::writeString (int address, const char* value)
{
  if (!value)
    return 0;

  if (address < 0 || (size_t)address > _size)
    return 0;

  size_t len;
  for (len = 0; len <= _size; len++)
    if (value[len] == 0)
      break;

  if (address + len > _size)
    return 0;

  memcpy(_data + address, (const uint8_t*) value, len + 1);
  _dirty = true;
  return strlen(value);
}

size_t EEPROMClass::writeString (int address, String value)
{
  return EEPROMClass::writeString (address, value.c_str());
}

size_t EEPROMClass::writeBytes (int address, const void* value, size_t len)
{
  if (!value || !len)
    return 0;

  if (address < 0 || address + len > _size)
    return 0;

  memcpy(_data + address, (const void*) value, len);
  _dirty = true;
  return len;
}

template <class T> T EEPROMClass::writeAll (int address, const T &value)
{
  if (address < 0 || address + sizeof(T) > _size)
    return value;

  memcpy(_data + address, (const uint8_t*) &value, sizeof(T));
  _dirty = true;

  return sizeof (value);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EEPROMClass EEPROM;
#endif
