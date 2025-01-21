#include "onboard_clock.h"

#include <cstdint>
#include <format>
#include <pico/aon_timer.h>

#include "main.h"
#include "status_manager.h"
#include "usb_communication.h"

void onboard_clock::clock_loop(CollectionData& collection_data)
{
  static uint8_t loops_since_ds_1307_check = 0;

  if (!aon_timer_is_running())
  {
    if (ds_1307::check_and_read_clock(collection_data.time_inst))
    {
      if (aon_timer_start_calendar(&collection_data.time_inst))
      {
        set_fault(status_manager::ONBOARD_CLOCK, false);
      }
      else
      {
        usb_communication::send_string("Fault: onboard clock not running");
        set_fault(status_manager::ONBOARD_CLOCK, true);
      }
    }
    else
    {
      usb_communication::send_string("Fault: onboard clock requires DS 1307 to be set in order to initialize");
      set_fault(status_manager::ONBOARD_CLOCK, true);
    }
    return;
  }

  const bool got_time = aon_timer_get_time_calendar(&collection_data.time_inst);
  // usb_communication::send_string(std::format("Onboard RTC: {} {} {} {} {} {} {}", collection_data.time_inst.tm_year,
  //                                            collection_data.time_inst.tm_mon, collection_data.time_inst.tm_mday,
  //                                            collection_data.time_inst.tm_wday, collection_data.time_inst.tm_hour,
  //                                            collection_data.time_inst.tm_min, collection_data.time_inst.tm_sec));

  if (!got_time)
  {
    usb_communication::send_string("Fault: Failed to get time from onboard clock");
    set_fault(status_manager::ONBOARD_CLOCK, true);
    return;
  }

  if (loops_since_ds_1307_check > 32)
  {
    if (ds_1307::functional_check(collection_data.time_inst))
    {
      loops_since_ds_1307_check = 0;
    }
  }
  else
  {
    loops_since_ds_1307_check++;
  }

  set_fault(status_manager::ONBOARD_CLOCK, false);
}
