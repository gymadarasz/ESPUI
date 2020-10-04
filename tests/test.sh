g++ -Wall -g test.cpp Tester.cpp ../lltoa.cpp ../cb_delay.cpp ../Template.cpp ../ESPUI.cpp helpers/helpers.cpp helpers/WString.cpp helpers/IPAddress.cpp helpers/HardwareSerial.cpp helpers/Print.cpp helpers/stdlib_noniso.c helpers/Stream.cpp -I./helpers -o test
./test
rm test
