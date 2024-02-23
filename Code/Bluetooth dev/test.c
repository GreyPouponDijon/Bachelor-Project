#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/*Devicetree aliases for LEDs and buttons*/
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define SW0_NODE  DT_ALIAS(sw0)
#define SW1_NODE  DT_ALIAS(sw1)


static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);


int setup(void)
{
	if (!((gpio_is_ready_dt(&led0)) & (gpio_is_ready_dt(&led1)) & (gpio_is_ready_dt(&button0)) & (gpio_is_ready_dt(&button1))))
	{	
		return 0;
	}
    // Configure port directions
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_configure_dt(&button1, GPIO_INPUT);
}

int main(void)
{

	setup();
	while(1){
	int but0_val = gpio_pin_get_dt(&button0);
	int but1_val = gpio_pin_get_dt(&button1);


	if (but0_val >= 0){
		gpio_pin_set_dt(&led0, but0_val);
	}

	if (but1_val >= 0){
		gpio_pin_set_dt(&led1, but1_val);
	}
	}
	return 0;
}
