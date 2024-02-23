/* 
 * Modified code from Blinky example
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define SW0_NODE DT_ALIAS(button0)
#define SW1_NODE DT_ALIAS(button1)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);


int setup(void)
{
	if ((!gpio_is_ready_dt(&led0)) & (!gpio_is_ready_dt(&led1)) & (!gpio_is_ready_dt(&sw0)) & (!gpio_is_ready_dt(&sw1))){
		return 0;
	}

	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	gpio_pin_configure_dt(&sw1, GPIO_INPUT);
}

int main(void)
{
	while(1){
	setup();

	
	if (gpio_pin_get_dt(&sw0)) {
		gpio_pin_toggle_dt(&led0);
	}

	if (gpio_pin_get_dt(&sw0)) {
		gpio_pin_toggle_dt(&led1)
	}
	}
	return 0;
}