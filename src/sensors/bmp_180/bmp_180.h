#pragma once

#define BMP_180_ADDR 0b1110111

#define BMP_180_RESET_VALUE 0xB6
#define BMP_180_DEVICE_ID 0x55

struct CollectionData;

namespace bmp_180
{
  constexpr double SEA_LEVEL_PRESS = 101325;

  namespace _reg_defs
  {
    // 2-byte registers
    constexpr uint8_t REG_CALIB_AC1 = 0xAA;
    constexpr uint8_t REG_CALIB_AC2 = 0xAC;
    constexpr uint8_t REG_CALIB_AC3 = 0xAE;
    constexpr uint8_t REG_CALIB_AC4 = 0xB0;
    constexpr uint8_t REG_CALIB_AC5 = 0xB2;
    constexpr uint8_t REG_CALIB_AC6 = 0xB4;
    constexpr uint8_t REG_CALIB_B1 = 0xB6;
    constexpr uint8_t REG_CALIB_B2 = 0xB8;
    constexpr uint8_t REG_CALIB_MB = 0xBA;
    constexpr uint8_t REG_CALIB_MC = 0xBC;
    constexpr uint8_t REG_CALIB_MD = 0xBE;

    constexpr uint8_t REG_OUT_XLSB = 0xF8;
    constexpr uint8_t REG_OUT_LSB = 0xF7;
    constexpr uint8_t REG_OUT_MSB = 0xF6;

    constexpr uint8_t REG_CTRL_MEAS = 0xF4;
    constexpr uint8_t REG_SCO = 0xF4;
    constexpr uint8_t REG_OSS = 0xF4;

    constexpr uint8_t REG_SOFT_RESET = 0xE0;
    constexpr uint8_t REG_DEVICE_ID = 0xD0;
  }

  struct CalibrationData
  {
    int16_t AC1, AC2, AC3;
    uint16_t AC4, AC5, AC6;
    int16_t B1, B2, MB, MC, MD;
  };

  enum oss_setting
  {
    ULTRA_LOW = 0x0,
    STANDARD = 0x1,
    HIGH = 0x2,
    ULTRA_HIGH = 0x3
  };

  inline CalibrationData bmp_180_calib_data{};

  bool check_device_id();
  bool read_calibration_data();
  bool read_press_temp_alt(oss_setting oss_setting, double& temperature, int32_t& pressure, double& altitude);
  bool soft_reset();
  bool is_conversion_complete(bool& is_complete);

  void send_calib_data();
  void pressure_loop(CollectionData & collection_data);
}
