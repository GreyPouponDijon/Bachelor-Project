#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include "../include/service.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <nrfx_timer.h>

LOG_MODULE_REGISTER(bt_gb, LOG_LEVEL_INF);

static struct bt_cb gb_cb;

enum {
	GB_C_INITIALIZED,
	GB_C_TX_NOTIF_ENABLED,
	GB_C_RX_WRITE_PENDING
};



static uint8_t on_received(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t length)
{
    struct bt_gb_client *gb;
    
    gb = CONTAINER_OF(params, struct bt_gb_client, tx_notif_params);

    if (!data) {
        LOG_DBG("Unsubscribed");
        params->value_handle = 0;
        atomic_clear_bit(&gb->state, GB_C_TX_NOTIF_ENABLED);
        if (gb->cb.unsubscribed) {
            gb->cb.unsubscribed(gb);
        }
        return BT_GATT_ITER_STOP;
    }
    LOG_DBG("notification data %p length %u", data, length);
    if (gb->cb.received) {
        return gb->cb.received(gb, data, length);
    }

    return BT_GATT_ITER_CONTINUE;
}

static void gb_ccc_cfg_changed(const struct bt_gatt_attr *attr, 
                                            uint16_t value)
{
    if(gb_cb.send_enabled){
        LOG_DBG("Notification has been turned on %s", value = BT_GATT_CCC_NOTIFY ? "on" : "off");
        gb_cb.send_enabled(value == BT_GATT_CCC_NOTIFY ?
			BT_GB_SEND_STATUS_ENABLED : BT_GB_SEND_STATUS_DISABLED);
    }
}

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_INF("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

	if (gb_cb.received) {
		gb_cb.received(conn, buf, len);
}
	return len;
}
static void on_sent(struct bt_conn *conn, void *user_data)
{
    LOG_DBG("Data send, conn %p", (void *)conn);

    if(gb_cb.sent){
        gb_cb.sent(conn);
    }
}

static void on_write(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
    struct bt_gb_client *gb;
    const void *data;
    uint16_t length;

    gb = CONTAINER_OF(params, struct bt_gb_client, rx_write_params);

    data = params->data;
    length = params->length;

    atomic_clear_bit(&gb->state, GB_C_RX_WRITE_PENDING);
    if (gb->cb.sent) {
        gb->cb.sent(gb, err, data, length);
    }
}

BT_GATT_SERVICE_DEFINE(gameboy_service,
BT_GATT_PRIMARY_SERVICE(BT_UUID_GB_SERVICE),
        BT_GATT_CHARACTERISTIC(BT_UUID_GB_ATTR_TX,
                               BT_GATT_CHRC_NOTIFY,
                               BT_GATT_PERM_READ,
                               NULL, NULL, NULL),
        BT_GATT_CCC(gb_ccc_cfg_changed,
                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
        BT_GATT_CHARACTERISTIC(BT_UUID_GB_ATTR_RX,
                              BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                              BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                              NULL, on_receive, NULL),
);

int bt_gb_init(struct bt_cb *callbacks)
{
    if(callbacks){
        gb_cb.received = callbacks->received;
        gb_cb.sent = callbacks->sent;
        gb_cb.send_enabled = callbacks->send_enabled;
    }
    return 0;
}

int bt_gb_client_init(struct bt_gb_client *gb, const struct bt_gb_client_init_param *gb_init)
{
    if (!gb || !gb_init) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&gb->state, GB_C_INITIALIZED)) {
		return -EALREADY;
	}

	memcpy(&gb->cb, &gb_init->cb, sizeof(gb->cb));

	return 0;
}

int bt_gb_send(struct bt_conn *conn, const uint16_t *data, uint16_t len)
{
    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &gameboy_service.attrs[2];

    params.attr = attr;
    params.data = data;
    params.len = len;
    params.func = on_sent;


    if (!conn) {
		LOG_DBG("Notification send to all connected peers");
		return bt_gatt_notify_cb(NULL, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify_cb(conn, &params);
    } else {
        return -EINVAL;
    }
}

int bt_gb_write(struct bt_gb_client *gb, const uint16_t *data, uint16_t len)
{
    int err;

    gb->rx_write_params.func = on_write;
    gb->rx_write_params.handle = gb->handles.rx;
    gb->rx_write_params.offset = 0;
    gb->rx_write_params.data = data;
    gb->rx_write_params.length = len;


    err = bt_gatt_write(gb->conn, &gb->rx_write_params);
    if (err) {
        LOG_INF("Failed to send, reason(%d)", err);
        atomic_clear_bit(&gb->state, GB_C_RX_WRITE_PENDING);
    }
    return err;
}

int bt_gb_handles_assign(struct bt_gatt_dm *dm, struct bt_gb_client *gb)
{
    const struct bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
    const struct bt_gatt_service_val *gatt_service = bt_gatt_dm_attr_service_val(gatt_service_attr);
    const struct bt_gatt_dm_attr *gatt_chrc;
    const struct bt_gatt_dm_attr *gatt_desc;

    LOG_INF("bt_gb_handles_assign");

    if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_GB_SERVICE)) {
        return -ENOTSUP;
    }
    LOG_DBG("Getting handles from GameBoy service.");
    memset(&gb->handles, 0xFF, sizeof(gb->handles));

    gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_GB_ATTR_TX);
    if (!gatt_chrc) {
        LOG_ERR("Missing GameBoy TX characteristic.");
        return -EINVAL;
    }

    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GB_ATTR_TX);
    if (!gatt_desc) {
        LOG_ERR("Missing GameBoy TX value descriptor in characteristic.");
        return -EINVAL;
    }
    LOG_DBG("Found handle for GameBoy Tx characteristic.");
    gb->handles.tx = gatt_desc->handle;

    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
    if (!gatt_desc) {
        LOG_ERR("Missing GameBoy TX CCC in characteristic.");
        return -EINVAL;
    }
    LOG_DBG("Found handle for CCC of GameBoy TX characteristic.");
    gb->handles.tx_ccc = gatt_desc->handle;

    gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_GB_ATTR_RX);
    if(!gatt_chrc) {
        LOG_ERR("Missing GameBoy RX characteristic.");
        return -EINVAL;
    }
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GB_ATTR_RX);
    if(!gatt_desc) {
        LOG_ERR("Missing GameBoy RX value descriptor in characteristic.");
        return -EINVAL;
    }
    LOG_DBG("Found handle for GameBoy RX characteristic");
    gb->handles.rx = gatt_desc->handle;

    gb->conn = bt_gatt_dm_conn_get(dm);
    return 0;
}

int bt_gb_subscribe_receive(struct bt_gb_client *gb)
{
    LOG_INF("bt_gb_subscribe_receive");
    int err;

    if (atomic_test_and_set_bit(&gb->state, GB_C_TX_NOTIF_ENABLED)) {
        return -EALREADY;
    }

    gb->tx_notif_params.notify = on_received;
    gb->tx_notif_params.value = BT_GATT_CCC_NOTIFY;
    gb->tx_notif_params.value_handle = gb->handles.tx;
    gb->tx_notif_params.ccc_handle = gb->handles.tx_ccc;
    atomic_set_bit(gb->tx_notif_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

    err = bt_gatt_subscribe(gb->conn, &gb->tx_notif_params);
    if (err) {
        LOG_ERR("Subscribe failed (err %d)", err);
        atomic_clear_bit(&gb->state, GB_C_TX_NOTIF_ENABLED);
    }   else {
        LOG_INF("Subscribed to: %u", gb->tx_notif_params.value_handle);
    }
    LOG_INF("done");

    return err;
}

