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

#ifndef BMI270_THINGS_H
#define BMI270_THINGS_H

extern float tiltValue;

extern bool nanFlag;

int SetSensorParam(void);

int ObtainPosition(void);

long map(float x, float in_min, float in_max, float out_min, float out_max);

#endif