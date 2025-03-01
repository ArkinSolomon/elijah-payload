#pragma once

#include "elijah_state_framework.h"
#include "data_type.h"

struct CollectionData
{
  uint64_t sequence;
  tm time_inst;
  int32_t pressure;
  double temperature;
  double altitude;
  double accel_x, accel_y, accel_z;
  double gyro_x, gyro_y, gyro_z;
  double bat_voltage, bat_percent;
};

enum class PayloadPersistentDataKey
{
  BAROMETRIC_PRESSURE,
  ACCEL_CALIB_X,
  ACCEL_CALIB_Y,
  ACCEL_CALIB_Z,
  GYRO_CALIB_X,
  GYRO_CALIB_Y,
  GYRO_CALIB_Z
};

class PayloadStateManager final : public ElijahStateFramework<CollectionData, PayloadPersistentDataKey>
{
public:
  PayloadStateManager(): ElijahStateFramework("Payload")
  {
    PersistentDataStorage<PayloadPersistentDataKey> persistent_data_storage = get_persistent_data_storage();
    persistent_data_storage.register_key(PayloadPersistentDataKey::BAROMETRIC_PRESSURE, "Barometric pressure",
                                         101325.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::ACCEL_CALIB_X, "Accelerometer calibration X", 0.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::ACCEL_CALIB_Y, "Accelerometer calibration Y", 0.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::ACCEL_CALIB_Z, "Accelerometer calibration Z", 0.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::GYRO_CALIB_X, "Gyroscope calibration X", 0.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::GYRO_CALIB_Y, "Gyroscope calibration Y", 0.0);
    persistent_data_storage.register_key(PayloadPersistentDataKey::GYRO_CALIB_Z, "Gyroscope calibration Z", 0.0);
    persistent_data_storage.finish_registration();

    finish_registration();
  }

protected:
  START_STATE_ENCODER(CollectionData)
     ENCODE_STATE(sequence, DataType::UINT64, "Sequence", "")
     ENCODE_STATE(time_inst, DataType::TIME, "Time", "")
     ENCODE_STATE(pressure, DataType::DOUBLE, "Pressure", "Pa")\
     ENCODE_STATE(temperature, DataType::DOUBLE, "Temperature", "°C")
     ENCODE_STATE(altitude, DataType::DOUBLE, "Altitude", "m")
     ENCODE_STATE(accel_x, DataType::DOUBLE, "Acceleration X", "m/s^2")
     ENCODE_STATE(accel_y, DataType::DOUBLE, "Acceleration Y", "m/s^2")
     ENCODE_STATE(accel_z, DataType::DOUBLE, "Acceleration Z", "m/s^2")
     ENCODE_STATE(gyro_x, DataType::DOUBLE, "Gyro X", "deg/s")
     ENCODE_STATE(gyro_y, DataType::DOUBLE, "Gyro Y", "deg/s")
     ENCODE_STATE(gyro_z, DataType::DOUBLE, "Gyro Z", "deg/s")
     ENCODE_STATE(bat_voltage, DataType::DOUBLE, "Voltage", "V")
     ENCODE_STATE(bat_percent, DataType::DOUBLE, "Battery percentage", "%")
   END_STATE_ENCODER()
};

inline auto payload_state_manager = PayloadStateManager();
