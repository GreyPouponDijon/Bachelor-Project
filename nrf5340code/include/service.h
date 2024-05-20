#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
/*
This code is loosely based on nus.h and nus_client.h
https://github.com/nrfconnect/sdk-nrf/blob/main/include/bluetooth/services/nus.h
https://github.com/nrfconnect/sdk-nrf/blob/main/include/bluetooth/services/nus_client.h
*/

#ifndef BT_SERVICE_H
#define BT_SERVICE_H

#define BT_UUID_SERVICE_VAL \
    BT_UUID_128_ENCODE(0xdea2ef30,0x9fb4,0x494c,0x9070,0xfd2e40ab523b)
#define BT_UUID_CHRC_TX_VAL \
    BT_UUID_128_ENCODE(0xdea2ef31,0x9fb4,0x494c,0x9070,0xfd2e40ab523b)

#define BT_UUID_CHRC_RX_VAL \
    BT_UUID_128_ENCODE(0xdea2ef32,0x9fb4,0x494c,0x9070,0xfd2e40ab523b)


#define BT_UUID_GB_SERVICE    BT_UUID_DECLARE_128(BT_UUID_SERVICE_VAL)
#define BT_UUID_GB_ATTR_TX BT_UUID_DECLARE_128(BT_UUID_CHRC_TX_VAL)
#define BT_UUID_GB_ATTR_RX BT_UUID_DECLARE_128(BT_UUID_CHRC_RX_VAL)


enum bt_gb_send_status {
	BT_GB_SEND_STATUS_ENABLED,
	BT_GB_SEND_STATUS_DISABLED,
};


//structure used for defining callbacks for peripheral
struct bt_cb {
    void (*received)(struct bt_conn *conn, const uint16_t *const data, uint16_t len);
    void (*sent)(struct bt_conn *conn);
    void (*send_enabled)(enum bt_gb_send_status status);
};

struct bt_gb_client;
//structure used for defining callbacks for central
struct bt_gb_client_cb {
    uint8_t (*received)(struct bt_gb_client *gb, const uint16_t *data, uint16_t len);
    void (*sent)(struct bt_gb_client *gb, uint8_t err, const uint16_t *data, uint16_t len);
    void (*unsubscribed)(struct bt_gb_client *gb);
};
//strucuture used in handle assign process
struct bt_gb_client_handles {

	uint16_t rx;
    uint16_t tx;
	uint16_t tx_ccc;
};

struct bt_gb_client_init_param {
	struct bt_gb_client_cb cb;
};
//structure used in bluetooth central carrying information about attributes on the connected service
struct bt_gb_client {
    struct bt_conn *conn;
    atomic_t state;
    struct bt_gb_client_handles handles;
    struct bt_gatt_subscribe_params tx_notif_params;
    struct bt_gatt_write_params rx_write_params;
    struct bt_gb_client_cb cb;
};
//protoypes of functions in service.c
int bt_gb_init(struct bt_cb *callbacks);

int bt_gb_client_init(struct bt_gb_client *gb, const struct bt_gb_client_init_param *gb_init);

int bt_gb_send(struct bt_conn *conn, const uint16_t *data, uint16_t len);

int bt_gb_write(struct bt_gb_client *gb, const uint16_t *data, uint16_t len);

int bt_gb_handles_assign(struct bt_gatt_dm *dm, struct bt_gb_client *gb);

int bt_gb_subscribe_receive(struct bt_gb_client *gb);

#endif
