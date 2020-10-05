g++ -Wall -g test.cpp Tester.cpp ../lltoa.cpp ../cb_delay.cpp ../Template.cpp ../ESPUI.cpp helpers/helpers.cpp helpers/WString.cpp helpers/IPAddress.cpp helpers/HardwareSerial.cpp helpers/Print.cpp helpers/stdlib_noniso.c helpers/Stream.cpp helpers/IPv6Address.cpp helpers/cbuf.cpp helpers/EEPROM/src/EEPROM.cpp helpers/WiFi/src/WiFiSTA.cpp helpers/WiFi/src/WiFiGeneric.cpp helpers/WiFi/src/WiFi.cpp -I./helpers -I./helpers/EEPROM/src -I./helpers/WiFi/src -o test
./test
rm test
