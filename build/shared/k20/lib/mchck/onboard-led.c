#include <mchck.h>

// uDAD R003
static const enum gpio_pin_id led_pin = GPIO_PTC4;
// uDAD R001
//static const enum gpio_pin_id led_pin = GPIO_PTD4;

void
onboard_led(enum onboard_led_state state)
{
        gpio_dir(led_pin, GPIO_OUTPUT);
        pin_mode(led_pin, PIN_MODE_DRIVE_HIGH);

        if (state == ONBOARD_LED_OFF || state == ONBOARD_LED_ON)
                gpio_write(led_pin, state);
        else
                gpio_toggle(led_pin);
}
