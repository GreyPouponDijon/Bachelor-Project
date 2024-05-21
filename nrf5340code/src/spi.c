#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <nrfx_timer.h>
#include "../include/service.h"

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/spi.h>

/*
* C file initiating and driving the SPI maste on the nRF5340
* Send data decided in main using tranceive 
* Code built on: https://github.com/too1/ncs-spi-master-slave-example
*/

#define LOG_MODULE_NAME SPI_COMS
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define MY_SPI_MASTER DT_NODELABEL(my_spi_master)
#define MY_SPI_MASTER_CS_DT_SPEC SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(reg_my_spi_master))



const struct device *spi_dev;
static struct k_poll_signal spi_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_done_sig);

//SPI initiation function
void spi_init(void)
{
	spi_dev = DEVICE_DT_GET(MY_SPI_MASTER);
	if(!device_is_ready(spi_dev)) {
		LOG_INF("SPI master device not ready!\n");
	}
	struct gpio_dt_spec spim_cs_gpio = MY_SPI_MASTER_CS_DT_SPEC;
	if(!device_is_ready(spim_cs_gpio.port)){
		LOG_INF("SPI master chip select device not ready!\n");
	}
}

static struct spi_config spi_cfg = {
	.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
	.frequency = 125000,
	.slave = 0,
	.cs = NULL,
};

// Spi write function
int spi_write_test_msg(uint8_t ValToSend)
{
	static uint8_t tx_buffer[1];
	static uint8_t rx_buffer[1]={1};

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

        tx_buffer[0] = ValToSend;
	
	// Reset signal
	k_poll_signal_reset(&spi_done_sig);
	
	// Start transaction
	int error = spi_transceive_signal(spi_dev, &spi_cfg, &tx, &rx, &spi_done_sig);
	if(error != 0){
		LOG_INF("SPI transceive error: %i\n", error);
		return error;
	}

        //LOG_INF("SPI TX: 0x%.2x\n", tx_buffer[0]);

	 //Wait for the done signal to be raised and log the rx buffer
	int spi_signaled, spi_result;
	do{
		k_poll_signal_check(&spi_done_sig, &spi_signaled, &spi_result);
	} while(spi_signaled == 0);
	//LOG_INF("SPI RX: 0x%.2x\n", rx_buffer[0]);
	return rx_buffer[0];
}

