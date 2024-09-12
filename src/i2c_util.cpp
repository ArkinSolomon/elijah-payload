#include "i2c_util.h"
#include "pin_outs.h"

bool i2c_util::read_byte(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t reg_addr, int8_t &output)
{
  uint8_t read_byte;
  const bool success = i2c_util::read_ubyte(i2c, dev_addr, reg_addr, read_byte);
  output = read_byte;
  return success;
}

bool i2c_util::read_ubyte(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t &output)
{
  return i2c_util::read_bytes(i2c, dev_addr, reg_addr, &output, 1);
}

bool i2c_util::read_short(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t reg_addr, int16_t &output)
{
  uint16_t read_ushort;
  const bool success = i2c_util::read_ushort(i2c, dev_addr, reg_addr, read_ushort);
  if (!success)
  {
    return false;
  }

  output = read_ushort;
  return true;
}

bool i2c_util::read_ushort(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t reg_addr, uint16_t &output)
{
  uint8_t read_data[2];
  const bool success = i2c_util::read_bytes(i2c, dev_addr, reg_addr, read_data, 2);
  if (!success)
  {
    return false;
  }

  output = (read_data[0] << 8) | read_data[1];
  return success;
}

/**
 * Read len bytes into output[].
 *
 * Returns false on failure, or true otherwise
 */
bool i2c_util::read_bytes(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t output[], uint8_t len)
{
  uint8_t read_bytes = 0;
  for (uint8_t offset = 0; offset < len; ++offset)
  {
    const uint8_t read_addr = reg_addr + offset;
    const int bytes_written = i2c_write_blocking(I2C_BUS, dev_addr, &read_addr, 1, true);
    if (bytes_written != 1)
    {
      return false;
    }

    const int bytes_read = i2c_read_blocking(I2C_BUS, dev_addr, output + offset, len, offset == len - 1);
    if (bytes_read != 1)
    {
      return false;
    }

    ++read_bytes;
  }

  return true;
}