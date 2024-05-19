#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <nrfx_timer.h>

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

#ifndef SPI_H
#define SPI_H

void spi_init(void);

int spi_write_test_msg(uint8_t ValToSend);

#endif