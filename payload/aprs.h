#pragma once

#include <memory>
#include <string>
#include <hardware/pwm.h>
#include <sys/_stdint.h>

#define APRS_FRAME_FLAG 0x7E

namespace aprs
{
  inline uint pwm_slice_num;
  inline pwm_chan pwm_channel;
  inline double pwm_freq;

  std::unique_ptr<uint8_t> encode_aprs_string_message(const std::string& callsign, const std::string& message, size_t& message_len);

  void init_aprs_system(uint32_t clock_speed_khz, uint8_t pwm_gpio, pwm_chan channel);
  void transmit_bytes(const uint8_t* data, size_t len, size_t flag_count);
}
