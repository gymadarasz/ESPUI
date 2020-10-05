/*
 WiFiSTA.cpp - WiFi library for esp32

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
#include "WiFiSTA.h"

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
#include <esp32-hal.h>
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Private functions ------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static bool sta_config_equal(const wifi_config_t& lhs, const wifi_config_t& rhs);


/**
 * compare two STA configurations
 * @param lhs station_config
 * @param rhs station_config
 * @return equal
 */
static bool sta_config_equal(const wifi_config_t& lhs, const wifi_config_t& rhs)
{
    if(memcmp(&lhs, &rhs, sizeof(wifi_config_t)) != 0) {
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- STA function -----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool WiFiSTAClass::_autoReconnect = true;
bool WiFiSTAClass::_useStaticIp = false;


void WiFiSTAClass::_setStatus(wl_status_t status)
{
    
}

/**
 * Return Connection status.
 * @return one of the value defined in wl_status_t
 *
 */
wl_status_t WiFiSTAClass::status()
{
    return (wl_status_t)0;
}

/**
 * Start Wifi connection
 * if passphrase is set the most secure supported mode will be automatically selected
 * @param ssid const char*          Pointer to the SSID string.
 * @param passphrase const char *   Optional. Passphrase. Valid characters in a passphrase must be between ASCII 32-126 (decimal).
 * @param bssid uint8_t[6]          Optional. BSSID / MAC of AP
 * @param channel                   Optional. Channel of AP
 * @param connect                   Optional. call connect
 * @return
 */
wl_status_t WiFiSTAClass::begin(const char* ssid, const char *passphrase, int32_t channel, const uint8_t* bssid, bool connect)
{

    if(!WiFi.enableSTA(true)) {
        return WL_CONNECT_FAILED;
    }

    if(!ssid || *ssid == 0x00 || strlen(ssid) > 31) {
        return WL_CONNECT_FAILED;
    }

    if(passphrase && strlen(passphrase) > 64) {
        return WL_CONNECT_FAILED;
    }

    wifi_config_t conf;
    memset(&conf, 0, sizeof(wifi_config_t));
    strcpy(reinterpret_cast<char*>(conf.sta.ssid), ssid);

    if(passphrase) {
        if (strlen(passphrase) == 64){ // it's not a passphrase, is the PSK
            memcpy(reinterpret_cast<char*>(conf.sta.password), passphrase, 64);
        } else {
            strcpy(reinterpret_cast<char*>(conf.sta.password), passphrase);
        }
    }

    if(bssid) {
        conf.sta.bssid_set = 1;
        memcpy((void *) &conf.sta.bssid[0], (void *) bssid, 6);
    }

    if(channel > 0 && channel <= 13) {
        conf.sta.channel = channel;
    }

    wifi_config_t current_conf;
    
    if(!sta_config_equal(current_conf, conf)) {
        


    } else if(status() == WL_CONNECTED){
        return WL_CONNECTED;
    } else {
        
    }





    return status();
}

wl_status_t WiFiSTAClass::begin(char* ssid, char *passphrase, int32_t channel, const uint8_t* bssid, bool connect)
{
    return begin((const char*) ssid, (const char*) passphrase, channel, bssid, connect);
}

/**
 * Use to connect to SDK config.
 * @return wl_status_t
 */
wl_status_t WiFiSTAClass::begin()
{
    if(!WiFi.enableSTA(true)) {
        return WL_CONNECT_FAILED;
    }

    return status();
}

/**
 * will force a disconnect an then start reconnecting to AP
 * @return ok
 */
bool WiFiSTAClass::reconnect()
{
    if(WiFi.getMode() & WIFI_MODE_STA) {
        
    }
    return false;
}

/**
 * Disconnect from the network
 * @param wifioff
 * @return  one value of wl_status_t enum
 */
bool WiFiSTAClass::disconnect(bool wifioff, bool eraseap)
{
    wifi_config_t conf;

    if(WiFi.getMode() & WIFI_MODE_STA){
        if(eraseap){
            memset(&conf, 0, sizeof(wifi_config_t));
            
        }
        
        if(wifioff) {
             return WiFi.enableSTA(false);
        }
        return true;
    }

    return false;
}

/**
 * Change IP configuration settings disabling the dhcp client
 * @param local_ip   Static ip configuration
 * @param gateway    Static gateway configuration
 * @param subnet     Static Subnet mask
 * @param dns1       Static DNS server 1
 * @param dns2       Static DNS server 2
 */
bool WiFiSTAClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2)
{
    if(!WiFi.enableSTA(true)) {
        return false;
    }

    return true;
}

/**
 * is STA interface connected?
 * @return true if STA is connected to an AD
 */
bool WiFiSTAClass::isConnected()
{
    return (status() == WL_CONNECTED);
}


/**
 * Setting the ESP32 station to connect to the AP (which is recorded)
 * automatically or not when powered on. Enable auto-connect by default.
 * @param autoConnect bool
 * @return if saved
 */
bool WiFiSTAClass::setAutoConnect(bool autoConnect)
{
    /*bool ret;
    ret = esp_wifi_set_auto_connect(autoConnect);
    return ret;*/
    return false;//now deprecated
}

/**
 * Checks if ESP32 station mode will connect to AP
 * automatically or not when it is powered on.
 * @return auto connect
 */
bool WiFiSTAClass::getAutoConnect()
{
    /*bool autoConnect;
    esp_wifi_get_auto_connect(&autoConnect);
    return autoConnect;*/
    return false;//now deprecated
}

bool WiFiSTAClass::setAutoReconnect(bool autoReconnect)
{
    _autoReconnect = autoReconnect;
    return true;
}

bool WiFiSTAClass::getAutoReconnect()
{
    return _autoReconnect;
}

/**
 * Wait for WiFi connection to reach a result
 * returns the status reached or disconnect if STA is off
 * @return wl_status_t
 */
uint8_t WiFiSTAClass::waitForConnectResult()
{
    //1 and 3 have STA enabled
    if((WiFiGenericClass::getMode() & WIFI_MODE_STA) == 0) {
        return WL_DISCONNECTED;
    }
    return status();
}

/**
 * Get the station interface IP address.
 * @return IPAddress station IP
 */
IPAddress WiFiSTAClass::localIP()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return IPAddress();
}


/**
 * Get the station interface MAC address.
 * @param mac   pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
 * @return      pointer to uint8_t *
 */
uint8_t* WiFiSTAClass::macAddress(uint8_t* mac)
{
    if(WiFiGenericClass::getMode() != WIFI_MODE_NULL){
        
    }
    return mac;
}

/**
 * Get the station interface MAC address.
 * @return String mac
 */
String WiFiSTAClass::macAddress(void)
{
    uint8_t mac[6] = {0};
    char macStr[18] = { 0 };
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        
    }
    else{
        
    }
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

/**
 * Get the interface subnet mask address.
 * @return IPAddress subnetMask
 */
IPAddress WiFiSTAClass::subnetMask()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return IPAddress();
}

/**
 * Get the gateway ip address.
 * @return IPAddress gatewayIP
 */
IPAddress WiFiSTAClass::gatewayIP()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return IPAddress();
}

/**
 * Get the DNS ip address.
 * @param dns_no
 * @return IPAddress DNS Server IP
 */
IPAddress WiFiSTAClass::dnsIP(uint8_t dns_no)
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return IPAddress();
}

/**
 * Get the broadcast ip address.
 * @return IPAddress broadcastIP
 */
IPAddress WiFiSTAClass::broadcastIP()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return WiFiGenericClass::calculateBroadcast(IPAddress(), IPAddress());
}

/**
 * Get the network id.
 * @return IPAddress networkID
 */
IPAddress WiFiSTAClass::networkID()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPAddress();
    }
    return WiFiGenericClass::calculateNetworkID(IPAddress(), IPAddress());
}

/**
 * Get the subnet CIDR.
 * @return uint8_t subnetCIDR
 */
uint8_t WiFiSTAClass::subnetCIDR()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return (uint8_t)0;
    }
    return WiFiGenericClass::calculateSubnetCIDR(IPAddress());
}

/**
 * Return the current SSID associated with the network
 * @return SSID
 */
String WiFiSTAClass::SSID() const
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return String();
    }

    return String();
}

/**
 * Return the current pre shared key associated with the network
 * @return  psk string
 */
String WiFiSTAClass::psk() const
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return String();
    }
    wifi_config_t conf;
    
    return String(reinterpret_cast<char*>(conf.sta.password));
}

/**
 * Return the current bssid / mac associated with the network if configured
 * @return bssid uint8_t *
 */
uint8_t* WiFiSTAClass::BSSID(void)
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return NULL;
    }
    
    return NULL;
}

/**
 * Return the current bssid / mac associated with the network if configured
 * @return String bssid mac
 */
String WiFiSTAClass::BSSIDstr(void)
{
    uint8_t* bssid = BSSID();
    if(!bssid){
        return String();
    }
    char mac[18] = { 0 };
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return String(mac);
}

/**
 * Return the current network RSSI.
 * @return  RSSI value
 */
int8_t WiFiSTAClass::RSSI(void)
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return 0;
    }

    return 0;
}

/**
 * Get the station interface Host name.
 * @return char array hostname
 */
const char * WiFiSTAClass::getHostname()
{
    const char * hostname = NULL;
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return hostname;
    }
    return hostname;
}

/**
 * Set the station interface Host name.
 * @param  hostname  pointer to const string
 * @return true on   success
 */
bool WiFiSTAClass::setHostname(const char * hostname)
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return false;
    }
    return true;
}

/**
 * Enable IPv6 on the station interface.
 * @return true on success
 */
bool WiFiSTAClass::enableIpV6()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return false;
    }
    return true;
}

/**
 * Get the station interface IPv6 address.
 * @return IPv6Address
 */
IPv6Address WiFiSTAClass::localIPv6()
{
    if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
        return IPv6Address();
    }
    return IPv6Address();
}


bool WiFiSTAClass::_smartConfigStarted = false;
bool WiFiSTAClass::_smartConfigDone = false;


bool WiFiSTAClass::beginSmartConfig() {
    if (_smartConfigStarted) {
        return false;
    }

    if (!WiFi.mode(WIFI_STA)) {
        return false;
    }

    return false;
}

bool WiFiSTAClass::stopSmartConfig() {
    if (!_smartConfigStarted) {
        return true;
    }

    return false;
}

bool WiFiSTAClass::smartConfigDone() {
    if (!_smartConfigStarted) {
        return false;
    }

    return _smartConfigDone;
}

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
const char * sc_status_strings[] = {
    "WAIT",
    "FIND_CHANNEL",
    "GETTING_SSID_PSWD",
    "LINK",
    "LINK_OVER"
};

const char * sc_type_strings[] = {
    "ESPTOUCH",
    "AIRKISS",
    "ESPTOUCH_AIRKISS"
};
#endif

void WiFiSTAClass::_smartConfigCallback(uint32_t st, void* result) {

}
