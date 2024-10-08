#pragma once

#include <cstdint>

#define DS_1307_ADDR 0b1101000

struct CollectionData;

namespace ds_1307
{

  namespace _reg_defs
  {
    constexpr uint8_t REG_SECONDS = 0x00;
    constexpr uint8_t REG_MINUTES = 0x01;
    constexpr uint8_t REG_HOURS = 0x02;
    constexpr uint8_t REG_DAY = 0x03;
    constexpr uint8_t REG_DATE = 0x04;
    constexpr uint8_t REG_MONTH = 0x05;
    constexpr uint8_t REG_YEAR = 0x06;
    constexpr uint8_t REG_CTRL = 0x07;
  }

  enum class custom_register
  {
     SEA_LEVEL_PRESS = 0x08,
  };

  enum day_of_week
  {
    DAY_NOT_SET = 0,
    SUNDAY = 1,
    MONDAY = 2,
    TUESDAY = 3,
    WEDNESDAY = 4,
    THURSDAY = 5,
    FRIDAY = 6,
    SATURDAY = 7
  };

  enum month_of_year
  {
    MONTH_NOT_SET = 0,
    JANUARY = 1,
    FEBRUARY = 2,
    MARCH = 3,
    APRIL = 4,
    MAY = 5,
    JUNE = 6,
    JULY = 7,
    AUGUST = 8,
    SEPTEMBER = 9,
    OCTOBER = 10,
    NOVEMBER = 11,
    DECEMBER = 12
  };

  struct TimeInstance
  {
    uint16_t year;
    month_of_year month;
    uint8_t date;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    day_of_week day;
  };

  bool check_clock(bool& clock_set);
  bool set_clock(uint16_t year, month_of_year month, day_of_week day, uint8_t date, uint8_t hours, uint8_t minutes, uint8_t seconds);
  bool get_time_instance(TimeInstance &time_inst);
  void load_blank_inst(TimeInstance &time_inst);
  void handle_time_set_packet(const uint8_t* packet_data);

  bool read_custom_register(custom_register addr, uint8_t* output, uint8_t size);
  bool write_custom_register(custom_register addr, const uint8_t* data, uint8_t size);
  void reg_dump();
  void erase_data();

  void clock_loop(CollectionData& collection_data);
}