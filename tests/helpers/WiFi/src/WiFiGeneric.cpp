/*
 ESP8266WiFiGeneric.cpp - WiFi library for esp8266

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Reworked on 28 Dec 2015 by Markus Sattler

 */

#include "WiFi.h"
#include "WiFiGeneric.h"

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include "esp_ipc.h"
#include "esp_wifi.h"


} //extern "C"

#include <vector>


static bool _start_network_event_task(){
    return false;
}

void tcpipInit(){
    static bool initialized = false;
    if(!initialized && _start_network_event_task()){
        initialized = true;
    }
}

static bool lowLevelInitDone = false;
static bool wifiLowLevelInit(bool persistent){
    if(!lowLevelInitDone){
        tcpipInit();
        
        
        
        if(!persistent){
            
        }
        lowLevelInitDone = true;
    }
    return true;
}

static bool wifiLowLevelDeinit(){
    //deinit not working yet!
    //esp_wifi_deinit();
    return true;
}

static bool _esp_wifi_started = false;

static bool espWiFiStart(bool persistent){
    if(_esp_wifi_started){
        return true;
    }
    if(!wifiLowLevelInit(persistent)){
        return false;
    }
    
    _esp_wifi_started = true;
    system_event_t event;
    event.event_id = SYSTEM_EVENT_WIFI_READY;
    WiFiGenericClass::_eventCallback(nullptr, &event);

    return true;
}

static bool espWiFiStop(){
    if(!_esp_wifi_started){
        return true;
    }
    _esp_wifi_started = false;
    
    return wifiLowLevelDeinit();
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Generic WiFi function -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

typedef struct WiFiEventCbList {
    static wifi_event_id_t current_id;
    wifi_event_id_t id;
    WiFiEventCb cb;
    WiFiEventFuncCb fcb;
    WiFiEventSysCb scb;
    system_event_id_t event;

    WiFiEventCbList() : id(current_id++), cb(NULL), fcb(NULL), scb(NULL), event(SYSTEM_EVENT_WIFI_READY) {}
} WiFiEventCbList_t;
wifi_event_id_t WiFiEventCbList::current_id = 1;


// arduino dont like std::vectors move static here
static std::vector<WiFiEventCbList_t> cbEventList;

bool WiFiGenericClass::_persistent = true;
wifi_mode_t WiFiGenericClass::_forceSleepLastMode = WIFI_MODE_NULL;

WiFiGenericClass::WiFiGenericClass()
{

}

int WiFiGenericClass::setStatusBits(int bits){
    return 0;
}

int WiFiGenericClass::clearStatusBits(int bits){
    return 0;
}

int WiFiGenericClass::getStatusBits(){
    return 0;
}

int WiFiGenericClass::waitStatusBits(int bits, uint32_t timeout_ms){
    return 0;
}

/**
 * set callback function
 * @param cbEvent WiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventCb cbEvent, system_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventFuncCb cbEvent, system_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = cbEvent;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventSysCb cbEvent, system_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = cbEvent;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

/**
 * removes a callback form event handler
 * @param cbEvent WiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
void WiFiGenericClass::removeEvent(WiFiEventCb cbEvent, system_event_id_t event)
{
    if(!cbEvent) {
        return;
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.cb == cbEvent && entry.event == event) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void WiFiGenericClass::removeEvent(WiFiEventSysCb cbEvent, system_event_id_t event)
{
    if(!cbEvent) {
        return;
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.scb == cbEvent && entry.event == event) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void WiFiGenericClass::removeEvent(wifi_event_id_t id)
{
    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.id == id) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

/**
 * callback for WiFi events
 * @param arg
 */
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
const char * system_event_names[] = { "WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP", "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT", "STA_WPS_ER_PIN", "AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};
#endif
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
const char * system_event_reasons[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT" };
#define reason2str(r) ((r>176)?system_event_reasons[r-176]:system_event_reasons[r-1])
#endif
esp_err_t WiFiGenericClass::_eventCallback(void *arg, system_event_t *event)
{
    if(event->event_id < 26) {
        
    }
    if(event->event_id == SYSTEM_EVENT_SCAN_DONE) {
        

    } else if(event->event_id == SYSTEM_EVENT_STA_START) {
        WiFiSTAClass::_setStatus(WL_DISCONNECTED);
        setStatusBits(STA_STARTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_STA_STOP) {
        WiFiSTAClass::_setStatus(WL_NO_SHIELD);
        clearStatusBits(STA_STARTED_BIT | STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
    } else if(event->event_id == SYSTEM_EVENT_STA_CONNECTED) {
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        setStatusBits(STA_CONNECTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        uint8_t reason = event->event_info.disconnected.reason;
        
        if(reason == WIFI_REASON_NO_AP_FOUND) {
            WiFiSTAClass::_setStatus(WL_NO_SSID_AVAIL);
        } else if(reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_ASSOC_FAIL) {
            WiFiSTAClass::_setStatus(WL_CONNECT_FAILED);
        } else if(reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
            WiFiSTAClass::_setStatus(WL_CONNECTION_LOST);
        } else if(reason == WIFI_REASON_AUTH_EXPIRE) {

        } else {
            WiFiSTAClass::_setStatus(WL_DISCONNECTED);
        }
        clearStatusBits(STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
        if(((reason == WIFI_REASON_AUTH_EXPIRE) ||
            (reason >= WIFI_REASON_BEACON_TIMEOUT && reason != WIFI_REASON_AUTH_FAIL)) &&
            WiFi.getAutoReconnect())
        {
            WiFi.disconnect();
            WiFi.begin();
        }
    } else if(event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        WiFiSTAClass::_setStatus(WL_CONNECTED);
        setStatusBits(STA_HAS_IP_BIT | STA_CONNECTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_STA_LOST_IP) {
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        clearStatusBits(STA_HAS_IP_BIT);

    } else if(event->event_id == SYSTEM_EVENT_AP_START) {
        setStatusBits(AP_STARTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_AP_STOP) {
        clearStatusBits(AP_STARTED_BIT | AP_HAS_CLIENT_BIT);
    } else if(event->event_id == SYSTEM_EVENT_AP_STACONNECTED) {
        setStatusBits(AP_HAS_CLIENT_BIT);
    } else if(event->event_id == SYSTEM_EVENT_AP_STADISCONNECTED) {
        

    } else if(event->event_id == SYSTEM_EVENT_ETH_START) {
        setStatusBits(ETH_STARTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_ETH_STOP) {
        clearStatusBits(ETH_STARTED_BIT | ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
    } else if(event->event_id == SYSTEM_EVENT_ETH_CONNECTED) {
        setStatusBits(ETH_CONNECTED_BIT);
    } else if(event->event_id == SYSTEM_EVENT_ETH_DISCONNECTED) {
        clearStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
    } else if(event->event_id == SYSTEM_EVENT_ETH_GOT_IP) {
        setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT);

    } else if(event->event_id == SYSTEM_EVENT_GOT_IP6) {
        
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.cb || entry.fcb || entry.scb) {
            if(entry.event == (system_event_id_t) event->event_id || entry.event == SYSTEM_EVENT_MAX) {
                if(entry.cb) {
                    entry.cb((system_event_id_t) event->event_id);
                } else if(entry.fcb) {
                    entry.fcb((system_event_id_t) event->event_id, (system_event_info_t) event->event_info);
                } else {
                    entry.scb(event);
                }
            }
        }
    }
    return ESP_OK;
}

/**
 * Return the current channel associated with the network
 * @return channel (1-13)
 */
int32_t WiFiGenericClass::channel(void)
{
    uint8_t primaryChan = 0;
    if(!lowLevelInitDone){
        return primaryChan;
    }
    
    return primaryChan;
}


/**
 * store WiFi config in SDK flash area
 * @param persistent
 */
void WiFiGenericClass::persistent(bool persistent)
{
    _persistent = persistent;
}


/**
 * set new mode
 * @param m WiFiMode_t
 */
bool WiFiGenericClass::mode(wifi_mode_t m)
{
    wifi_mode_t cm = getMode();
    if(cm == m) {
        return true;
    }
    if(!cm && m){
        if(!espWiFiStart(_persistent)){
            return false;
        }
    } else if(cm && !m){
        return espWiFiStop();
    }
    
    return true;
}

/**
 * get WiFi mode
 * @return WiFiMode
 */
wifi_mode_t WiFiGenericClass::getMode()
{
    return (wifi_mode_t)0;
}

/**
 * control STA mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::enableSTA(bool enable)
{

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_STA) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return mode((wifi_mode_t)(currentMode | WIFI_MODE_STA));
        }
        return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_STA)));
    }
    return true;
}

/**
 * control AP mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::enableAP(bool enable)
{

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_AP) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return mode((wifi_mode_t)(currentMode | WIFI_MODE_AP));
        }
        return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_AP)));
    }
    return true;
}

/**
 * control modem sleep when only in STA mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::setSleep(bool enable)
{
    if((getMode() & WIFI_MODE_STA) == 0){
        
        return false;
    }
    return false;
}

/**
 * get modem sleep enabled
 * @return true if modem sleep is enabled
 */
bool WiFiGenericClass::getSleep()
{
    if((getMode() & WIFI_MODE_STA) == 0){
        
        return false;
    }
    
    return false;
}

/**
 * control wifi tx power
 * @param power enum maximum wifi tx power
 * @return ok
 */
bool WiFiGenericClass::setTxPower(wifi_power_t power){
    if((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0){
        
        return false;
    }
    return false;
}

wifi_power_t WiFiGenericClass::getTxPower(){
    return (wifi_power_t)0;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Generic Network function ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/**
 * DNS callback
 * @param name
 * @param ipaddr
 * @param callback_arg
 */


/**
 * Resolve the given hostname to an IP address.
 * @param aHostname     Name to be resolved
 * @param aResult       IPAddress structure to store the returned IP address
 * @return 1 if aIPAddrString was successfully converted to an IP address,
 *          else error code
 */
int WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult)
{
    aResult = static_cast<uint32_t>(0);
    waitStatusBits(WIFI_DNS_IDLE_BIT, 5000);
    clearStatusBits(WIFI_DNS_IDLE_BIT);
    
    setStatusBits(WIFI_DNS_IDLE_BIT);
    if((uint32_t)aResult == 0){
        
    }
    return (uint32_t)aResult != 0;
}

IPAddress WiFiGenericClass::calculateNetworkID(IPAddress ip, IPAddress subnet) {
	IPAddress networkID;

	for (size_t i = 0; i < 4; i++)
		networkID[i] = subnet[i] & ip[i];

	return networkID;
}

IPAddress WiFiGenericClass::calculateBroadcast(IPAddress ip, IPAddress subnet) {
    IPAddress broadcastIp;
    
    for (int i = 0; i < 4; i++)
        broadcastIp[i] = ~subnet[i] | ip[i];

    return broadcastIp;
}

uint8_t WiFiGenericClass::calculateSubnetCIDR(IPAddress subnetMask) {
	uint8_t CIDR = 0;

	for (uint8_t i = 0; i < 4; i++) {
		if (subnetMask[i] == 0x80)  // 128
			CIDR += 1;
		else if (subnetMask[i] == 0xC0)  // 192
			CIDR += 2;
		else if (subnetMask[i] == 0xE0)  // 224
			CIDR += 3;
		else if (subnetMask[i] == 0xF0)  // 242
			CIDR += 4;
		else if (subnetMask[i] == 0xF8)  // 248
			CIDR += 5;
		else if (subnetMask[i] == 0xFC)  // 252
			CIDR += 6;
		else if (subnetMask[i] == 0xFE)  // 254
			CIDR += 7;
		else if (subnetMask[i] == 0xFF)  // 255
			CIDR += 8;
	}

	return CIDR;
}
