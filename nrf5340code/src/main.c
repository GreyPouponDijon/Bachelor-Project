/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2022 - 2023, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <nrfx_timer.h>
#include "../include/service.h"
#include "../include/spi.h"
#include "../include/positionCalc.h"
#include <zephyr/drivers/spi.h>

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


//Bluetooth device
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

//Log module
#define LOG_MODULE_NAME gameboy_cartridge
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);


//Timing MACROS

/** @brief Symbol specifying sleep time for bluetooth in main and CS (Chip Select)*/
#define MAIN_INTERVAL 50

/** @brief Symbol specifying period of CS (Chip Select) */
#define PERIOD_CS 17

/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_MS 1UL

/** @brief Symbol for setting stack size for threads. 1024 chosen as pow(2,10) is a safe size.  */
#define STANDARD_STACK_SIZE 1024

/** @brief Symbol for setting stack priority. By using the same priority and yield all threads are able to run.*/
#define STANDARD_PRIORITY 0

/** @brief Set Pin 1.07 as CS driver */
#define CS_GPIO DT_ALIAS(led0)

/** @brief Connect Devicetree information and alias */
static const struct gpio_dt_spec CS = GPIO_DT_SPEC_GET(CS_GPIO, gpios);


//SPI Variables

/** @brief Define S_OUT from GBP*/
#define MY_SPI_SLAVE  DT_NODELABEL(my_spi_slave)

/** @brief Define SPI slave tx buffer */
static uint8_t slave_tx_buffer[2]={0x00,0x00};

/** @brief Define SPI slave receive buffer.
 *  Global variable due to write in interupt and access in spi_trancieve function to achieve
 *  correct timing with communicating with the gameboy*/
static uint8_t slave_rx_buffer[2]={0x00,0x00};

/** @brief Counter for timing on spi master send*/
volatile uint32_t TimerCounter;

/** @brief Value that shows current position of BMI270.*/
volatile uint8_t DirectionValue;

/** @brief Value sent to recipient through bluetooth.*/
static uint16_t user_value = 0;

volatile uint16_t recived_value = 0;

/** @brief Value to adjust send rate on the SPI to the Gameboy.*/
volatile unsigned long MSCounter = 0;

bool reciving = false;



	int ret;
	bool ledstatus = false;
	uint32_t counter = 0;


static struct bt_conn *current_conn;
static struct bt_conn *default_conn;
static struct bt_gb_client gb_client;
typedef enum {
        peripheral,
        central,
} bt_mode_toggle_type;

bt_mode_toggle_type bt_mode_toggle;

 


K_THREAD_STACK_DEFINE(obtainVal_stack, STANDARD_STACK_SIZE);

struct k_thread obtainVal_id;
//advertising parameters
static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SERVICE_VAL), 
};

bt_mode_toggle_type toggle_mode_type(bt_mode_toggle_type currentType) {
    return (currentType == peripheral) ? central : peripheral;
}

int start_peripheral(void);
int start_central(void);


static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
        struct bt_gb_client *gb = context;
        LOG_INF("Service discovery complete");

        bt_gatt_dm_data_print(dm);
        bt_gb_handles_assign(dm, gb);
        bt_gb_subscribe_receive(gb);
        
        bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
        LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
        LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
        .completed = discovery_complete,
        .service_not_found = discovery_service_not_found,
        .error_found = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
        int err;

        if (conn != default_conn){
                return;
        }

        err = bt_gatt_dm_start(conn, BT_UUID_GB_SERVICE, &discovery_cb, &gb_client);
        if (err) {
                LOG_ERR("Could not start the discovery prodcedure, error (%d)", err);
        }
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		LOG_INF("MTU exchange done");
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{       
        char addr[BT_ADDR_LE_STR_LEN];

        if (err) {
                LOG_ERR("Connection failed (err %u)", err);
                return;
                if (bt_mode_toggle == central){
                        err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
                        if (err) {
                                LOG_ERR("Scanning failed to start (err %d)", err);
                        }
                }
                return;
        }
        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
        LOG_INF("Connected %s", addr);

        static struct bt_gatt_exchange_params exchange_params;

        exchange_params.func = exchange_func;
        err = bt_gatt_exchange_mtu(conn, &exchange_params);
        if (err) {
              LOG_WRN("MTU exchange failed (err %d)", err);  
        }

        err = bt_scan_stop();
        if ((!err) && (err != -EALREADY)) {
                LOG_ERR("Stop LE scan failed (err %d)", err);
        }

        current_conn = bt_conn_ref(conn);

        gatt_discover(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
        char addr[BT_ADDR_LE_STR_LEN];
        int err;

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

        LOG_INF("Disconnected: %s (reason %u)", addr, reason);

        if(default_conn != conn){
                return;
        }
        if (bt_mode_toggle == central){
                bt_conn_unref(default_conn);
                default_conn = NULL;
                err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
                if (err) {
                        LOG_ERR("Scanning failed to start (err %d)", err);
                }
        } else { 
                bt_conn_unref(current_conn);
                current_conn = NULL;
        }      
}

static void scan_filter_match(struct bt_scan_device_info *device_info, struct bt_scan_filter_match *filter_match, bool connectable)
{
        char addr[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

        LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
        LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn)
{
        LOG_INF("scan_connecting");
        default_conn = bt_conn_ref(conn);
}

static void filter_no_match(struct bt_scan_device_info *device_info,
				bool connectable)
{
        char addr[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
        LOG_INF("Filters not matched. Address: %s", addr);
}

static void ble_data_sent(struct bt_gb_client *gb, uint8_t err, const uint16_t *const data, uint16_t len)
{
        ARG_UNUSED(gb);
        ARG_UNUSED(len);
        LOG_INF("ble_data_sent");
        if (err) {
                LOG_WRN("ATT error code:0x%02X", err);
        } else {
                LOG_INF("Data sent: %u", *data);
        }
}

static uint8_t ble_data_received(struct bt_gb_client *gb, const uint16_t *data, uint16_t len)
{
        ARG_UNUSED(gb);
        ARG_UNUSED(len);

        LOG_INF("ble_data_received");
        LOG_INF("Data received %u", *data);
        return BT_GATT_ITER_CONTINUE;
}

static void bt_receive_cb(struct bt_conn *conn, const uint16_t *const data, uint16_t len)
{
    char addr[BT_ADDR_LE_STR_LEN] =  {0};

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
    recived_value = *data;

    LOG_INF("Received data: %u, from %s", recived_value, addr);
};

static int bt_client_init(void)
{
        int err;
        struct bt_gb_client_init_param init = {
                .cb = {
                        .received = ble_data_received,
                        .sent = ble_data_sent,
                }
        };

        err = bt_gb_client_init(&gb_client, &init);

        if (err) {
                LOG_ERR("Gameboy client initialization failed (err %d)", err);
                return err;
        }
        LOG_INF("Gameboy client initialized");
        return err;
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, filter_no_match, scan_connecting_error, scan_connecting);

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
};

static int scan_init(void)
{
        int err;
        struct bt_scan_init_param scan_init = {
                .connect_if_match = 1,
        };

        bt_scan_init(&scan_init);
        bt_scan_cb_register(&scan_cb);

        err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_GB_SERVICE);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

        err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
        if (err) {
                LOG_ERR("Filters cannot be turned on (err %d)", err);
                return err;
        }

        LOG_INF("Scan module initialized");
        return err;
}

static struct bt_cb gb_cb = {
    .received = bt_receive_cb,
};


void mode_toggle()
{
        switch (bt_mode_toggle)
        {
        case central:
                start_central();
                break;

        case peripheral:
                start_peripheral();
                break;

        default:
                break;
        }
}

void mode_change_recived(uint8_t mode)
{
        if(mode == 0xFD){
                LOG_INF("Mode change received");
                bt_mode_toggle = toggle_mode_type(bt_mode_toggle);
                mode_toggle();
                slave_rx_buffer[1]--;

        }

        if(reciving == true){
                reciving = false;
                LOG_INF("Sending");
                if(bt_mode_toggle == peripheral){
                        int err = bt_gb_send(NULL, &user_value, sizeof(user_value));
                        if (err) {
                                LOG_ERR("Failed to send (%d)", err);
                        }
                } else if (bt_mode_toggle == central){
                        int err = bt_gb_write(&gb_client, &user_value, sizeof(user_value));
                        if (err) {
                                LOG_ERR("Failed to send (%d)", err);
                        }
                }
        }
        }


int start_central(void)
{
        int err;
        err = scan_init();
        if (err){
                LOG_ERR("scan_init failed (err %d)", err);
                return err;
        }

        err = bt_client_init();
        if (err) {
                LOG_ERR("bt_gb_client_init failed (err %d)", err);
                return err;
        }
        err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
        if (err) {
                LOG_ERR("Scanning failed to start (err %d)", err);
                return err;
        }
        LOG_INF("Scanning started");
        return 0;
}

int start_peripheral(void)
{
        int err;
        err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
        if (err) {
                LOG_ERR("Advertising failed to start (%d)", err);
                return err;
        }

        LOG_INF("Advertising has started");
        return 0;
}

void GameboyReciveAndSend(int sendData, volatile uint8_t tiltData)
{
        TimerCounter++; 
        if(TimerCounter > 8)
        {
                MSCounter++;
                if(MSCounter >= tiltValue)
                {
                        spi_write_test_msg(tiltData);
                        MSCounter = 0;
                }

                else
                {
                        spi_write_test_msg(sendData);
                }
                TimerCounter = 0;
                slave_rx_buffer[1]--;
        }
}

static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
        
    if(event_type == NRF_TIMER_EVENT_COMPARE0)
    {
        if( slave_rx_buffer[1] == 0xFE)
        {
               TimerCounter++;
               if(TimerCounter <= 1)
               {
                        spi_write_test_msg(DirectionValue);  
                        TimerCounter = 0;
                        slave_rx_buffer[1]--;
                        slave_rx_buffer[0]--;
               }
        }

        if(slave_rx_buffer[1] == 0x69)
        {
                if(recived_value < user_value && user_value >= 10)
                {
                        GameboyReciveAndSend(0x00,0x77);
                }
                
                else if (recived_value > user_value && user_value >= 10)
                {
                        GameboyReciveAndSend(0x00,0x13);
                }
                else
                {
                        GameboyReciveAndSend(0x00,DirectionValue);
                }
                
        }

        if(slave_rx_buffer[1] != 0x68 && slave_rx_buffer[1] != 0x69 && slave_rx_buffer[1] != 0xFE && 
           slave_rx_buffer[1] != 0xFD && slave_rx_buffer[1] != 0xFC && slave_rx_buffer[1] != 0x00 &&
           slave_rx_buffer[1] != 0x26 && slave_rx_buffer[1] != 0x49 && slave_rx_buffer[1] != 0x34 && 
           slave_rx_buffer[1] != 0xD2 && slave_rx_buffer[1] != 0xA4 && slave_rx_buffer[1] != 0x4D && 
           slave_rx_buffer[1] != 0x93 && slave_rx_buffer[1] != 0x9A && slave_rx_buffer[1] != 0xAA)
        {
                reciving = true;
                user_value = slave_rx_buffer[1];
                GameboyReciveAndSend(0x00,DirectionValue);
                slave_rx_buffer[1]=0x00;
        }
        

        if(counter >= PERIOD_CS)
        {
                counter = 0;
                ret =gpio_pin_set_dt(&CS, 0);
                ledstatus = !ledstatus;
                if (ret < 0) {
                return 0;
                }
        }
        else
        {
                ret = gpio_pin_set_dt(&CS,1);
                ledstatus = !ledstatus;
                if (ret < 0) {
                        return 0;
                }	
                counter++;
        }
        
    }
}



void nrfx_enable(void)
{
	nrfx_err_t status;
    (void)status;

	nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";

	status = nrfx_timer_init(&timer_inst, &config, timer_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

	#if defined(__ZEPHYR__)
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                       NRFX_TIMER_INST_HANDLER_GET(TIMER_INST_IDX), 0);
	#endif

	 nrfx_timer_clear(&timer_inst);

    /* Creating variable desired_ticks to store the output of nrfx_timer_ms_to_ticks function */
    uint32_t desired_ticks = nrfx_timer_ms_to_ticks(&timer_inst, TIME_TO_WAIT_MS);
    //printk("Time to wait: %lu ms", TIME_TO_WAIT_MS);

    /*
     * Setting the timer channel NRF_TIMER_CC_CHANNEL0 in the extended compare mode to clear the timer and
     * trigger an interrupt if internal counter register is equal to desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    nrfx_timer_enable(&timer_inst);
    //printk("Timer status: %s", nrfx_timer_is_enabled(&timer_inst) ? "enabled" : "disabled");
}

// SPI slave functionality

//SPI slave device
const struct device *spi_slave_dev;

//K poll signal for SPI slave
static struct k_poll_signal spi_slave_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_slave_done_sig);

//SPI slave configuration
static const struct spi_config spi_slave_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
				 SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
	.frequency = 8192,
	.slave = 0,
};

//SPI slave initialization
void spi_slave_init(void)
{
	spi_slave_dev = DEVICE_DT_GET(MY_SPI_SLAVE);
	if(!device_is_ready(spi_slave_dev)) {
		LOG_INF("SPI slave device not ready!\n");
	}
}


int spi_slave_write_test_msg(void)
{


	const struct spi_buf s_tx_buf = {
		.buf = slave_tx_buffer,
		.len = sizeof(slave_tx_buffer)
	};
	const struct spi_buf_set s_tx = {
		.buffers = &s_tx_buf,
		.count = 1
	};

	struct spi_buf s_rx_buf = {
		.buf = slave_rx_buffer,
		.len = sizeof(slave_rx_buffer),
	};
	const struct spi_buf_set s_rx = {
		.buffers = &s_rx_buf,
		.count = 1
	};

	// Reset signal
	k_poll_signal_reset(&spi_slave_done_sig);
	
	// Start transaction
	int error = spi_transceive_signal(spi_slave_dev, &spi_slave_cfg, &s_tx, &s_rx, &spi_slave_done_sig);
	if(error != 0){
		LOG_INF("SPI slave transceive error: %i\n", error);
		return error;
	}
	return 0;
}

int spi_slave_check_for_message(void)
{
	int signaled, result;
	k_poll_signal_check(&spi_slave_done_sig, &signaled, &result);
	if(signaled != 0){
		return 0;
	}
	else return -1;
}




int change_gpio_voltage(uint32_t target_voltage)
{
    int err = 0;
    
    uint32_t regout = NRF_UICR->VREGHVOUT;
    LOG_INF("REGOUT = 0x%08x", regout);
    LOG_INF("%d", target_voltage);    
    if ((regout & UICR_VREGHVOUT_VREGHVOUT_Msk)  != target_voltage) {
        LOG_INF("Target voltage not set. Configuring");

        // Set NVMC in write mode:
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->CONFIG != NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos) {
            // Wait...
        }

        // Write the actual UICR Register:
        NRF_UICR->VREGHVOUT = (target_voltage | ~UICR_VREGHVOUT_VREGHVOUT_Msk);
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
            // Wait...
        }

        // Set NVMC back in read mode:
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->CONFIG != NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos) {
            // Wait...
        }
        // Check whether it was set successfully:
        if ((NRF_UICR->VREGHVOUT & UICR_VREGHVOUT_VREGHVOUT_Msk) != target_voltage) {
            err = 0;
        }
        // Reset if success. Config will remain on future reboots.
        if (err == 0) {
            NVIC_SystemReset();
        }
    }
    return err;
}



//Threads

int obtainVal(void)
{
        LOG_INF("Obtaining value");
        nrfx_enable();
        SetSensorParam();
        while(1)
        {
                k_sleep(K_MSEC(10));
                DirectionValue = ObtainPosition();
                k_yield();
        }
}

void startObtainVal(void)
{
        k_thread_create(&obtainVal_id, obtainVal_stack, STANDARD_STACK_SIZE, obtainVal, NULL, NULL, NULL, STANDARD_PRIORITY, 0, K_NO_WAIT);
        k_thread_start(&obtainVal_id);
}


int SPICheckForMessage(void)
{    
        spi_init();

	spi_slave_init();

        LOG_INF("Starting SPI");

        spi_slave_write_test_msg();
        while (1)
        {

		if(spi_slave_check_for_message() == 0){
                // Prepare the next SPI slave transaction
                LOG_INF("Received: %d", slave_rx_buffer[1]);
                spi_slave_write_test_msg();
		}
        k_sleep(K_MSEC(10));
        k_yield();
        }
        
 
}

K_THREAD_DEFINE(SPICheckForMessage_id, STANDARD_STACK_SIZE, SPICheckForMessage, NULL, NULL, NULL, STANDARD_PRIORITY, 0, 0);





int main(void)
{
        startObtainVal();
        LOG_INF("Starting GameBoy Cartridge");
        int err = 0;
        int blink_status = 0;
        int ret;

        err = bt_enable(NULL);
        if (err) {
                LOG_ERR("Failed to enable bluetooth (%d)", err);
                return err;
        }
        err = bt_gb_init(&gb_cb);
        if (err) {
                LOG_ERR("Failed to initialize GameBoy service (%d)", err);
        }
        if (!device_is_ready(CS.port)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&CS, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

        change_gpio_voltage(UICR_VREGHVOUT_VREGHVOUT_3V0);

        start_peripheral();

        for (;;) {
                k_sleep(K_MSEC(MAIN_INTERVAL));
                mode_change_recived(slave_rx_buffer[1]);
                if (nanFlag == true){
                nanFlag = false;
                LOG_INF("Nan detected");
                k_thread_abort(&obtainVal_id);
                k_sleep(K_MSEC(MAIN_INTERVAL));
                startObtainVal();
                }
                k_yield();
        }   	
}
