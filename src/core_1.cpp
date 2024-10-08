#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "pin_outs.h"
#include "core_1.h"

void launch_core_1()
{
  multicore_launch_core1(core_1_main);
  multicore_fifo_push_blocking(MC_FLAG_VALUE);
}

void core_1_main()
{
  uint32_t g = multicore_fifo_pop_blocking();

  if (g != MC_FLAG_VALUE)
    return;

  multicore_fifo_push_blocking(MC_FLAG_VALUE);

  while (true)
  {
    gpio_put(TEMP_LED_PIN, true);
    sleep_ms(200);
    gpio_put(TEMP_LED_PIN, false);
    sleep_ms(800);
  }
}
