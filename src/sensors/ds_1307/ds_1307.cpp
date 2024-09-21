#include "ds_1307.h"

#include <cstdio>

#include "../i2c/i2c_util.h"
#include "../../pin_outs.h"
#include "hardware/i2c.h"
#include "src/status_manager.h"
#include "src/usb_communication.h"

/**
 * Check if the clock is detected,
 *
 * Returns false if the device is not detected. Set is set to true if the clock is detected AND has been set.
 */
bool ds_1307::check_clock(bool& clock_set)
{
  uint8_t seconds_reg;
  const bool seconds_read_success = i2c_util::read_ubyte(I2C_BUS, DS_1307_ADDR, _reg_defs::REG_SECONDS,
                                                         seconds_reg);
  if (!seconds_read_success)
  {
    clock_set = false;
    return false;
  }

  TimeInstance time_inst{};
  get_time_instance(time_inst);

  clock_set = (seconds_reg & 0x80) == 0 && time_inst.year > 0;
  return true;
}

/**
 * Set the clock.
 *
 * Use 24-hour time.
 */
bool ds_1307::set_clock(uint16_t year, const month_of_year month, const day_of_week day, const uint8_t date,
                        const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  const uint8_t seconds_data = seconds / 10 << 4 | seconds % 10;
  const uint8_t minutes_data = minutes / 10 << 4 | minutes % 10;
  const uint8_t hours_data = hours / 10 << 4 | hours % 10;
  const uint8_t date_data = date / 10 << 4 | date % 10;
  const uint8_t month_data = static_cast<uint8_t>(month) / 10 << 4 | month % 10;

  year -= 2000;
  const uint8_t year_data = static_cast<uint8_t>(year) / 10 << 4 | year % 10;
  const uint8_t write_data[8] = {
    _reg_defs::REG_SECONDS, seconds_data, minutes_data, hours_data, static_cast<uint8_t>(day), date_data, month_data,
    year_data
  };
  const bool success = i2c_write_blocking_until(I2C_BUS, DS_1307_ADDR, write_data, 8, false,
                                                delayed_by_ms(get_absolute_time(), 100));

  return success;
}

/**
 * Get the current time as an instance.
 */
bool ds_1307::get_time_instance(TimeInstance& time_inst)
{
  uint8_t reg_data[7];
  const bool success = i2c_util::read_bytes(I2C_BUS, DS_1307_ADDR, _reg_defs::REG_SECONDS, reg_data, 7);
  if (!success)
  {
    return false;
  }

  const uint8_t seconds = reg_data[0];
  const uint8_t minutes = reg_data[1];
  const uint8_t hours = reg_data[2];
  uint8_t day = reg_data[3];
  const uint8_t date = reg_data[4];
  const uint8_t month = reg_data[5];
  const uint8_t year = reg_data[6];

  time_inst.seconds = (seconds >> 4 & 0x7) * 10 + (seconds & 0xF);
  time_inst.minutes = (minutes >> 4) * 10 + (minutes & 0xF);
  time_inst.hours = (hours >> 4 & 0x3) * 10 + (hours & 0xF);
  time_inst.date = (date >> 4 & 0x3) * 10 + (date & 0xF);
  time_inst.day = static_cast<day_of_week>(day);
  time_inst.month = static_cast<month_of_year>(((month & 0x1) >> 4) * 10 + (month & 0xF));
  time_inst.year = 2000 + (year >> 4) * 10 + (year & 0xF);

  return true;
}

/**
 * Reset all values in the instance to 0.
 */
void ds_1307::load_blank_inst(TimeInstance& time_inst)
{
  time_inst.year = 0;
  time_inst.month = MONTH_NOT_SET;
  time_inst.date = 0;
  time_inst.hours = 0;
  time_inst.minutes = 0;
  time_inst.seconds = 0;
  time_inst.day = DAY_NOT_SET;
}

void ds_1307::handle_time_set_packet(const uint8_t* packet_data)
{
  const uint8_t seconds = packet_data[0];
  const uint8_t minutes = packet_data[1];
  const uint8_t hours = packet_data[2];
  const auto day = static_cast<day_of_week>(packet_data[3]);
  const uint8_t date = packet_data[4];
  const auto month = static_cast<month_of_year>(packet_data[5]);
  const uint16_t year = packet_data[6] << 8 | packet_data[7];

  const bool clock_did_set = set_clock(year, month, day, date, hours, minutes, seconds);
  if (!clock_did_set)
  {
    usb_communication::send_string("Fault: DS 1307 [SET_FAIL]");
    set_fault(status_manager::fault_id::DEVICE_DS_1307, true);
    send_packet(usb_communication::TIME_SET_FAIL);
  }
  else
  {
    set_fault(status_manager::fault_id::DEVICE_DS_1307, false);
    send_packet(usb_communication::TIME_SET_SUCCESS);
  }

}
