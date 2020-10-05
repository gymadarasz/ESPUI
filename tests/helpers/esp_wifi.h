// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/*               Notes about WiFi Programming
 *
 *  The esp32 WiFi programming model can be depicted as following picture:
 *
 *
 *                            default handler              user handler
 *  -------------             ---------------             ---------------
 *  |           |   event     |             | callback or |             |
 *  |   tcpip   | --------->  |    event    | ----------> | application |
 *  |   stack   |             |     task    |  event      |    task     |
 *  |-----------|             |-------------|             |-------------|
 *                                  /|\                          |
 *                                   |                           |
 *                            event  |                           |
 *                                   |                           |
 *                                   |                           |
 *                             ---------------                   |
 *                             |             |                   |
 *                             | WiFi Driver |/__________________|
 *                             |             |\     API call
 *                             |             |
 *                             |-------------|
 *
 * The WiFi driver can be consider as black box, it knows nothing about the high layer code, such as
 * TCPIP stack, application task, event task etc, all it can do is to receive API call from high layer
 * or post event queue to a specified Queue, which is initialized by API esp_wifi_init().
 *
 * The event task is a daemon task, which receives events from WiFi driver or from other subsystem, such
 * as TCPIP stack, event task will call the default callback function on receiving the event. For example,
 * on receiving event SYSTEM_EVENT_STA_CONNECTED, it will call tcpip_adapter_start() to start the DHCP
 * client in it's default handler.
 *
 * Application can register it's own event callback function by API esp_event_init, then the application callback
 * function will be called after the default callback. Also, if application doesn't want to execute the callback
 * in the event task, what it needs to do is to post the related event to application task in the application callback function.
 *
 * The application task (code) generally mixes all these thing together, it calls APIs to init the system/WiFi and
 * handle the events when necessary.
 *
 */

#ifndef __ESP_WIFI_H__
#define __ESP_WIFI_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_private/esp_wifi_private.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ERR_WIFI_NOT_INIT    (ESP_ERR_WIFI_BASE + 1)   /*!< WiFi driver was not installed by esp_wifi_init */
#define ESP_ERR_WIFI_NOT_STARTED (ESP_ERR_WIFI_BASE + 2)   /*!< WiFi driver was not started by esp_wifi_start */
#define ESP_ERR_WIFI_NOT_STOPPED (ESP_ERR_WIFI_BASE + 3)   /*!< WiFi driver was not stopped by esp_wifi_stop */
#define ESP_ERR_WIFI_IF          (ESP_ERR_WIFI_BASE + 4)   /*!< WiFi interface error */
#define ESP_ERR_WIFI_MODE        (ESP_ERR_WIFI_BASE + 5)   /*!< WiFi mode error */
#define ESP_ERR_WIFI_STATE       (ESP_ERR_WIFI_BASE + 6)   /*!< WiFi internal state error */
#define ESP_ERR_WIFI_CONN        (ESP_ERR_WIFI_BASE + 7)   /*!< WiFi internal control block of station or soft-AP error */
#define ESP_ERR_WIFI_NVS         (ESP_ERR_WIFI_BASE + 8)   /*!< WiFi internal NVS module error */
#define ESP_ERR_WIFI_MAC         (ESP_ERR_WIFI_BASE + 9)   /*!< MAC address is invalid */
#define ESP_ERR_WIFI_SSID        (ESP_ERR_WIFI_BASE + 10)   /*!< SSID is invalid */
#define ESP_ERR_WIFI_PASSWORD    (ESP_ERR_WIFI_BASE + 11)  /*!< Password is invalid */
#define ESP_ERR_WIFI_TIMEOUT     (ESP_ERR_WIFI_BASE + 12)  /*!< Timeout error */
#define ESP_ERR_WIFI_WAKE_FAIL   (ESP_ERR_WIFI_BASE + 13)  /*!< WiFi is in sleep state(RF closed) and wakeup fail */
#define ESP_ERR_WIFI_WOULD_BLOCK (ESP_ERR_WIFI_BASE + 14)  /*!< The caller would block */
#define ESP_ERR_WIFI_NOT_CONNECT (ESP_ERR_WIFI_BASE + 15)  /*!< Station still in disconnect status */

#define ESP_ERR_WIFI_POST        (ESP_ERR_WIFI_BASE + 18)  /*!< Failed to post the event to WiFi task */
#define ESP_ERR_WIFI_INIT_STATE  (ESP_ERR_WIFI_BASE + 19)  /*!< Invalod WiFi state when init/deinit is called */
#define ESP_ERR_WIFI_STOP_STATE  (ESP_ERR_WIFI_BASE + 20)  /*!< Returned when WiFi is stopping */

/**
 * @brief WiFi stack configuration parameters passed to esp_wifi_init call.
 */
typedef struct {
    system_event_handler_t event_handler;          /**< WiFi event handler */
    wifi_osi_funcs_t*      osi_funcs;              /**< WiFi OS functions */
    wpa_crypto_funcs_t     wpa_crypto_funcs;       /**< WiFi station crypto functions when connect */
    int                    static_rx_buf_num;      /**< WiFi static RX buffer number */
    int                    dynamic_rx_buf_num;     /**< WiFi dynamic RX buffer number */
    int                    tx_buf_type;            /**< WiFi TX buffer type */
    int                    static_tx_buf_num;      /**< WiFi static TX buffer number */
    int                    dynamic_tx_buf_num;     /**< WiFi dynamic TX buffer number */
    int                    csi_enable;             /**< WiFi channel state information enable flag */
    int                    ampdu_rx_enable;        /**< WiFi AMPDU RX feature enable flag */
    int                    ampdu_tx_enable;        /**< WiFi AMPDU TX feature enable flag */
    int                    nvs_enable;             /**< WiFi NVS flash enable flag */
    int                    nano_enable;            /**< Nano option for printf/scan family enable flag */
    int                    tx_ba_win;              /**< WiFi Block Ack TX window size */
    int                    rx_ba_win;              /**< WiFi Block Ack RX window size */
    int                    wifi_task_core_id;      /**< WiFi Task Core ID */
    int                    beacon_max_len;         /**< WiFi softAP maximum length of the beacon */
    int                    mgmt_sbuf_num;          /**< WiFi management short buffer number, the minimum value is 6, the maximum value is 32 */
    int                    magic;                  /**< WiFi init magic number, it should be the last field */
} wifi_init_config_t;

#ifdef CONFIG_ESP32_WIFI_STATIC_TX_BUFFER_NUM
#define WIFI_STATIC_TX_BUFFER_NUM CONFIG_ESP32_WIFI_STATIC_TX_BUFFER_NUM
#else
#define WIFI_STATIC_TX_BUFFER_NUM 0
#endif

#ifdef CONFIG_ESP32_WIFI_DYNAMIC_TX_BUFFER_NUM
#define WIFI_DYNAMIC_TX_BUFFER_NUM CONFIG_ESP32_WIFI_DYNAMIC_TX_BUFFER_NUM
#else
#define WIFI_DYNAMIC_TX_BUFFER_NUM 0
#endif

#if CONFIG_ESP32_WIFI_CSI_ENABLED
#define WIFI_CSI_ENABLED         1
#else
#define WIFI_CSI_ENABLED         0
#endif

#if CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED
#define WIFI_AMPDU_RX_ENABLED        1
#else
#define WIFI_AMPDU_RX_ENABLED        0
#endif

#if CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
#define WIFI_AMPDU_TX_ENABLED        1
#else
#define WIFI_AMPDU_TX_ENABLED        0
#endif

#if CONFIG_ESP32_WIFI_NVS_ENABLED
#define WIFI_NVS_ENABLED          1
#else
#define WIFI_NVS_ENABLED          0
#endif

#if CONFIG_NEWLIB_NANO_FORMAT
#define WIFI_NANO_FORMAT_ENABLED  1
#else
#define WIFI_NANO_FORMAT_ENABLED  0
#endif

extern const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs;

#define WIFI_INIT_CONFIG_MAGIC    0x1F2F3F4F

#ifdef CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
#define WIFI_DEFAULT_TX_BA_WIN CONFIG_ESP32_WIFI_TX_BA_WIN
#else
#define WIFI_DEFAULT_TX_BA_WIN 0 /* unused if ampdu_tx_enable == false */
#endif

#ifdef CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED
#define WIFI_DEFAULT_RX_BA_WIN CONFIG_ESP32_WIFI_RX_BA_WIN
#else
#define WIFI_DEFAULT_RX_BA_WIN 0 /* unused if ampdu_rx_enable == false */
#endif

#if CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1
#define WIFI_TASK_CORE_ID 1
#else
#define WIFI_TASK_CORE_ID 0
#endif

#ifdef CONFIG_ESP32_WIFI_SOFTAP_BEACON_MAX_LEN
#define WIFI_SOFTAP_BEACON_MAX_LEN CONFIG_ESP32_WIFI_SOFTAP_BEACON_MAX_LEN
#else
#define WIFI_SOFTAP_BEACON_MAX_LEN 752
#endif

#ifdef CONFIG_ESP32_WIFI_MGMT_SBUF_NUM
#define WIFI_MGMT_SBUF_NUM CONFIG_ESP32_WIFI_MGMT_SBUF_NUM
#else
#define WIFI_MGMT_SBUF_NUM 32
#endif

#define WIFI_INIT_CONFIG_DEFAULT() { \
    .event_handler = &esp_event_send, \
    .osi_funcs = &g_wifi_osi_funcs, \
    .wpa_crypto_funcs = g_wifi_default_wpa_crypto_funcs, \
    .static_rx_buf_num = CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM,\
    .dynamic_rx_buf_num = CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM,\
    .tx_buf_type = CONFIG_ESP32_WIFI_TX_BUFFER_TYPE,\
    .static_tx_buf_num = WIFI_STATIC_TX_BUFFER_NUM,\
    .dynamic_tx_buf_num = WIFI_DYNAMIC_TX_BUFFER_NUM,\
    .csi_enable = WIFI_CSI_ENABLED,\
    .ampdu_rx_enable = WIFI_AMPDU_RX_ENABLED,\
    .ampdu_tx_enable = WIFI_AMPDU_TX_ENABLED,\
    .nvs_enable = WIFI_NVS_ENABLED,\
    .nano_enable = WIFI_NANO_FORMAT_ENABLED,\
    .tx_ba_win = WIFI_DEFAULT_TX_BA_WIN,\
    .rx_ba_win = WIFI_DEFAULT_RX_BA_WIN,\
    .wifi_task_core_id = WIFI_TASK_CORE_ID,\
    .beacon_max_len = WIFI_SOFTAP_BEACON_MAX_LEN, \
    .mgmt_sbuf_num = WIFI_MGMT_SBUF_NUM, \
    .magic = WIFI_INIT_CONFIG_MAGIC\
};


#ifdef __cplusplus
}
#endif

#endif /* __ESP_WIFI_H__ */
