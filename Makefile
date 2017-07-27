ARDUINO_VARIANT = nodemcu
SERIAL_PORT = /dev/cu.SLAB_USBtoUART
# uncomment and set the right serail baud according to your sketch (default to 115200)
#SERIAL_BAUD = 115200
# uncomment this to use the 1M SPIFFS mapping
#SPIFFS_SIZE = 1
ARDUINO_LIBS = ESP8266WiFi
USER_DEFINE = -D_SSID_=\"YourSSID\" -D_WIFI_PASSWORD_=\"YourPassword\"
OTA_IP = 192.168.1.184
OTA_PORT = 8266 
OTA_AUTH = password
include /Users/ahihi/Code/repos/Esp8266-Arduino-Makefile/espXArduino.mk

