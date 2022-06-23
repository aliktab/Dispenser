/*
  Dispenser

  Copyright (C) 2014 Alik <aliktab@gmail.com> All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/


#include "application.h"
#include <cmath>
#include <ArduinoJson.h>


// firmware settings
PRODUCT_ID(16122);
PRODUCT_VERSION(1001);


// ROM constants
const long ROM_VERSION_NUMBER      = 4;

const long ROM_ADDR_VERSION        = 0x000;
const long ROM_ADDR_CURR_DATA      = 0x010;


// changes threshold for publish sensors
const long  MAX_PAUSE_BETWEEN_LOG  = 60 * 60;  // sec
const long  MIN_PAUSE_BETWEEN_LOG  = 5;        // sec
const long  LOG_THRESHOLD_ON_OFF   = 1;
const float LOG_THRESHOLD_VOLUME   = 0.1;      // litre


// pins definition
const uint8_t WATER_COUNTER_PIN    = D2;
const uint8_t WATER_RELAY_PIN      = D3;
const uint8_t DRAIN_RELAY_PIN      = D4;

const uint8_t ALL_RELAY_PINS[]     = { WATER_RELAY_PIN, DRAIN_RELAY_PIN };


// basic constants and variables
const time_t DELAY_DEBOUNCE_TIME   = 25;  // msec
const time_t DELAY_MAIN_CYCLE      = 25;  // msec
const time_t DELAY_WATER_DRAIN     = 12;  // sec

const float LITERS_IN_COUNT        = 0.005f;  // litre  #TMP


// pin states
volatile bool  water_tick_state   = true;
volatile float water_tick_value   = 0;

volatile time_t drain_start_time = 0;


// declare functions
void on_tick_pin_changed();

int set_volume(String _arg);


// data storage
struct PublicData
{
  PublicData()
  {
    age    = 0;

    vcl    = 0;
    soc    = 0;
    pow    = 1;

    water  = 0;
    drain  = 0;
    
    ticks  = 0.0;
    volume = 0.0;
    total  = 0.0;

    _frb   = false;
  }

  void reset()
  {
    water  = 0;
    drain  = 0;

    ticks  = 0.0;
    volume = 0.0;

    _frb   = false;
  }

  void load_rom(int _addr)
  {
    uint16_t version = ROM_VERSION_NUMBER;
    EEPROM.get(ROM_ADDR_VERSION, version);
    if (version != ROM_VERSION_NUMBER)
      save_rom(_addr);
    else
      EEPROM.get(_addr, *this);
  }

  void save_rom(int _addr) const
  {
    uint16_t version = ROM_VERSION_NUMBER;
    EEPROM.put(ROM_ADDR_VERSION, version);
    EEPROM.put(_addr, *this);
  }

  void attach_particle()
  {
    CloudClass::variable("water", water);
    CloudClass::variable("drain", drain);

    CloudClass::variable("ticks", ticks);

    CloudClass::variable("volume", volume);
    CloudClass::function("volume", set_volume);

    CloudClass::variable("total", total);
  }

  bool publish_particle(const char * _name)
  {
    StaticJsonBuffer<1024> json;
    JsonObject & root = json.createObject();

    root["age"] = age;

    root["vcl"] = round(vcl * 100.0f) / 100.0f;
    root["soc"] = round(soc * 10.0f) / 10.0f;
    root["pow"] = pow;

    root["water"]  = water;
    root["drain"]  = drain;

    root["ticks"]  = ticks;
    root["volume"] = round(volume * 1000.0f) / 1000.0f;
    root["total"]  = round(total * 1000.0f) / 1000.0f;

    char string_to_publish[1024];
    root.printTo(string_to_publish);

    _frb = false;

    return Particle.publish(_name, string_to_publish, NO_ACK);
  }

  bool is_changed(const PublicData & _cmp) const
  {
    if (_frb) return true;

    if (MIN_PAUSE_BETWEEN_LOG > abs(age - _cmp.age))
      return false;

    if (MAX_PAUSE_BETWEEN_LOG < abs(age - _cmp.age))
      return true;

    return  (LOG_THRESHOLD_ON_OFF  <= abs(water  - _cmp.water))  ||
            (LOG_THRESHOLD_ON_OFF  <= abs(drain  - _cmp.drain))  ||

            (LOG_THRESHOLD_VOLUME  <= abs(volume - _cmp.volume)) ||
            (LOG_THRESHOLD_VOLUME  <= abs(total  - _cmp.total));
  }

  void force_immediate_publish()
  {
    _frb = true;
  }

  time_t   age;

  double   vcl;
  double   soc;
  long     pow;

  int      water;
  int      drain;

  double   ticks;
  double   volume;

  double   total;

  bool     _frb;
};

PublicData current_data;
PublicData published_data;


// power control libs   https://github.com/particle-iot/PowerShield
#include "PowerShield.h"
PowerShield fuel_gauge;


// setup() runs once, when the device is first turned on.
void setup()
{
  // initialize charger & fuel gauge
  fuel_gauge.begin();
  fuel_gauge.quickStart();

  // read from ROM
  current_data.load_rom(ROM_ADDR_CURR_DATA);
  current_data.reset();

  // init Particle Cloud connect
  current_data.attach_particle();

  // init pins
  pinMode(WATER_COUNTER_PIN, INPUT_PULLUP);

  for (const uint8_t & pin : ALL_RELAY_PINS)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // attach interrupts
  attachInterrupt(WATER_COUNTER_PIN, on_tick_pin_changed, CHANGE);
}


// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  // get time data
  current_data.age = max(TimeClass::now(), published_data.age);

  // get power data
  current_data.vcl = fuel_gauge.getVCell();
  current_data.soc = fuel_gauge.getSoC();


  // update water values
  if (current_data.volume > 0.0f)
  {
    drain_start_time = 0;
    current_data.water = 1;
  }

  if (0 == drain_start_time)
  {
    current_data.total  += water_tick_value;
    current_data.volume -= water_tick_value;
  }
  water_tick_value = 0;

  if (current_data.volume < 0.0f)
  {
    current_data.volume = 0.0f;
    drain_start_time = TimeClass::now();

    current_data.water = 0;
    current_data.drain = 1;
  }

  if (TimeClass::now() - drain_start_time > DELAY_WATER_DRAIN)
  {
    drain_start_time   = 0;
    current_data.drain = 0;
  }


  // update valves
  digitalWrite(WATER_RELAY_PIN, current_data.water > 0 ? HIGH : LOW);
  digitalWrite(WATER_RELAY_PIN, current_data.drain > 0 ? HIGH : LOW);


  // save and send data to server if it was significantly changed
  bool must_save_data_to_rom = false;
  if (current_data.is_changed(published_data))
  {
    must_save_data_to_rom = true;

    current_data.publish_particle("data");
    published_data = current_data;
  }


  // save changed data to ROM
  if (must_save_data_to_rom)
    current_data.save_rom(ROM_ADDR_CURR_DATA);


  // pause
  delay(DELAY_MAIN_CYCLE);
}


void on_tick_pin_changed()
{
  if (water_tick_state != digitalRead(WATER_COUNTER_PIN))
  {
    delayMicroseconds(DELAY_DEBOUNCE_TIME * 1000);

    bool new_pin_state = (bool)digitalRead(WATER_COUNTER_PIN);
    if (water_tick_state != new_pin_state)
    {
      water_tick_value    += LITERS_IN_COUNT;
      water_tick_state = new_pin_state;
    }
  }
}


int set_volume(String _arg)
{
  current_data.volume = _arg.toFloat();
  current_data.save_rom(ROM_ADDR_CURR_DATA);

  current_data.force_immediate_publish();

  return 0;
}
