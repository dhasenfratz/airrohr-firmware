// WLAN config
#define WIFI_SSID "...." // provide SSID
#define WIFI_PWD "...."    // provide password
#define WIFI_CONNECT_RETRIES 40

// Services to feed data with
#define SEND_TO_LUFTDATEN 0
#define SEND_TO_MADAVI 0
#define SEND_TO_OPENSENSEMAP 0
#define SEND_TO_CSV 0
#define SEND_TO_INFLUX 0
#define SEND_TO_MQTT 1

#define SEND_INTERVAL_MS 145000

// NTP Server
#define NTP_SERVER "0.europe.pool.ntp.org"

// OpenSenseMap
#define HOST_OPENSENSEMAP "ingress.opensensemap.org"
#define URL_OPENSENSEMAP "/boxes/ID_OPENSENSEMAP/data?luftdaten=1"
#define PORT_OPENSENSEMAP 443

// Mavadi
#define HOST_MADAVI "api-rrd.madavi.de"
#define URL_MADAVI "/data.php"
#define PORT_MADAVI 443

// Luftdaten
#define HOST_LUFTDATEN "api.luftdaten.info"
#define URL_LUFTDATEN "/v1/push-sensor-data/"
#define PORT_LUFTDATEN 443

// InfluxDB (local)
#define HOST_INFLUX "api.example.com" // provide InfluxDB host
#define URL_INFLUX "/write?db=data" // provide InfluxDB url
#define PORT_INFLUX 8086 // provide InfluxDB port
#define USER_INFLUX "" // provide InfluxDB user
#define PWD_INFLUX "" // provide InfluxDB password

// MQTT
#define MQTT_BROKER "test.mosquitto.org"
#define MQTT_TOPIC "ESP8266" // ESP8266's id is added as subtopic
#define MQTT_PORT 1883

// DHT22, temperature and humidtiy sensor
#define DHT_ENABLED 1
#define DHT_TYPE DHT22
#define DHT_API_PIN 7
#define DHT_PIN D7

// SDS011, PM2.5 sensor
#define SDS_ENABLED 1
#define SDS_API_PIN 1
// Serial confusion: These definitions are based on SoftSerial
// TX (transmitting) pin on one side goes to RX (receiving) pin on other side
// SoftSerial RX PIN is D1 and goes to SDS TX
// SoftSerial TX PIN is D2 and goes to SDS RX
#define SDS_PIN_RX D1
#define SDS_PIN_TX D2
#define SDS_SAMPLE_TIME_MS 1000
#define SDS_WARMUP_TIME_MS 15000
#define SDS_READING_TIME_MS 5000

#define WATHDOG_TIMER_MS 30000

const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";

// Internal LED of ESP8266-12E is connected to GPIO2
#define LED_GPIO 2

// Debug level setting
#define DEBUG_LEVEL 3
// Debug levels
#define DEBUG_ERROR 1
#define DEBUG_WARN 2
#define DEBUG_INFO 3
#define DEBUG_DEBUG 4
