#include <format>
#include <hardware/gpio.h>
#include <pico/flash.h>
#include <pico/multicore.h>

#include "bmp_280.h"
#include "core1.h"
#include "i2c_util.h"
#include "mpu_6050.h"
#include "override_state_manager.h"
#include "pin_outs.h"
#include "sensors.h"
#include "../payload/status_manager.h"

int main()
{
  flash_safe_execute_core_init();
  multicore_lockout_victim_init();

  pin_init();

  StateFrameworkLogger::init_driver_on_core();

  core1::launch_core1();

  uint8_t core_data;
  queue_remove_blocking(&core1::core1_ready_queue, &core_data);
  override_state_manager = new OverrideStateManager();

  core_data = 0xBB;
  queue_add_blocking(&core1::core0_ready_queue, &core_data);

  sensors_init();

  OverrideState state{};

  uint d = 0;
  while (true)
  {
    bmp280->read_calibration_data();
    override_state_manager->check_for_commands();
    mpu6050->get_data(state.accel_x, state.accel_y, state.accel_z, state.gyro_x, state.gyro_y,
                      state.gyro_z);
    bmp280->read_press_temp_alt(state.pressure, state.temperature, state.altitude);

    state.bat_voltage = battery->get_voltage();
    state.bat_percent = battery->calc_charge_percent(state.bat_voltage);

    override_state_manager->state_changed(state);

    d++;
    if (d % 100 == 0)
    {
      gpio_put(LED_3_PIN, true);
      override_state_manager->set_fault(FaultKey::BMP280, true, std::format("Test!! {}", d));
    }

    gpio_put(LED_2_PIN, override_state_manager->is_faulted(FaultKey::BMP280));
    override_state_manager->log_message(std::format("slp = {}, bytes= {}",
                                                    override_state_manager->get_persistent_data_storage()->get_double(
                                                    OverridePersistentStateKey::SeaLevelPressure), override_state_manager->get_persistent_data_storage()->get_total_byte_size()), LogLevel::Debug);

    // gpio_put(LED_3_PIN, false);
    override_state_manager->lock_logger();
    override_state_manager->get_logger()->flush_write_buff();
    // gpio_put(LED_3_PIN, override_state_manager->get_logger()->write_full_buff());
    override_state_manager->release_logger();

    sleep_ms(50);
  }
}
