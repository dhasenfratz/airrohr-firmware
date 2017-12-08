#include <Arduino.h>

/*****************************************************************
/*                                                               *
/*  This source code needs to be compiled for the board          *
/*  NodeMCU 1.0 (ESP-12E Module)                                 *
/*                                                               *
/*****************************************************************
/* OK LAB Particulate Matter Sensor                              *
/*      - nodemcu-LoLin board                                    *
/*      - Nova SDS0111                                           *
/*  ﻿http://inovafitness.com/en/Laser-PM2-5-Sensor-SDS011-35.html *
/*                                                               *
/* Wiring Instruction:                                           *
/*      - SDS011 Pin 1  (TX)   -> Pin D1 / GPIO5                 *
/*      - SDS011 Pin 2  (RX)   -> Pin D2 / GPIO4                 *
/*      - SDS011 Pin 3  (GND)  -> GND                            *
/*      - SDS011 Pin 4  (2.5m) -> unused                         *
/*      - SDS011 Pin 5  (5V)   -> VU                             *
/*      - SDS011 Pin 6  (1m)   -> unused                         *
/*                                                               *
/*****************************************************************
/* Extension: DHT22 (AM2303)                                     *
/*  ﻿http://www.aosong.com/en/products/details.asp?id=117         *
/*                                                               *
/* DHT22 Wiring Instruction                                      *
/* (left to right, front is perforated side):                    *
/*      - DHT22 Pin 1 (VDD)     -> Pin 3V3 (3.3V)                *
/*      - DHT22 Pin 2 (DATA)    -> Pin D7 (GPIO13)               *
/*      - DHT22 Pin 3 (NULL)    -> unused                        *
/*      - DHT22 Pin 4 (GND)     -> Pin GND                       *
/*                                                               *
/*****************************************************************/

/*****************************************************************
/* Includes                                                      *
/*****************************************************************/
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <DHT.h>

#include "constants.h"

long int sample_count = 0;

/*****************************************************************
/* SDS011 declarations                                           *
/*****************************************************************/
SoftwareSerial serialSDS(SDS_PIN_RX, SDS_PIN_TX, false, 128);

/*****************************************************************
/* DHT declaration                                               *
/*****************************************************************/
DHT dht(DHT_PIN, DHT_TYPE);

bool send_now = false;
unsigned long start_time_ms;
unsigned long start_SDS_time_ms;
unsigned long act_micro;
unsigned long current_time_ms;
unsigned long last_micro = 0;
unsigned long min_micro = 1000000000; // TODO: use official min value from limits.h
unsigned long max_micro = 0;
unsigned long diff_micro = 0;

bool is_SDS_running = true;

unsigned long sending_time = 0;
bool send_failed = false;

int sds_pm10_sum = 0;
int sds_pm25_sum = 0;
int sds_val_count = 0;
int sds_pm10_max = 0;
int sds_pm10_min = 20000; // TODO: use official min value from limits.h
int sds_pm25_max = 0;
int sds_pm25_min = 20000; // TODO: use official min value from limits.h

String last_value_SDS_P1 = "";
String last_value_SDS_P2 = "";
String last_value_DHT_T = "";
String last_value_DHT_H = "";

String esp_chip_id;
String server_name;

String basic_auth_influx = "";

long last_page_load = millis();
bool first_csv_line = true;

String data_first_part = "{\"software_version\": \"" + String(SOFTWARE_VERSION) + "\", \"sensordatavalues\":[";

/*****************************************************************
/* Debug output                                                  *
/*****************************************************************/
void debug_out(const String& text, const int level, const bool linebreak) {
	if (level <= DEBUG_LEVEL) {
		if (linebreak) {
			Serial.println(text);
		} else {
			Serial.print(text);
		}
	}
}

/*****************************************************************
/* IPAddress to String                                           *
/*****************************************************************/
String ip_to_string(const IPAddress& ip) {
	char str[24];
	sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return String(str);
}

/*****************************************************************
/* convert float to string with a                                *
/* precision of two decimal places                               *
/*****************************************************************/
String float_to_string(const float value) {
	char temp[15];
	String s;

	dtostrf(value, 13, 2, temp);
	s = String(temp);
	s.trim();
	return s;
}

/*****************************************************************
/* convert value to json string                                  *
/*****************************************************************/
String value_to_json(const String& type, const String& value) {
	String s = F("{\"value_type\":\"{t}\",\"value\":\"{v}\"},");
	s.replace("{t}", type); s.replace("{v}", value);
	return s;
}

/*****************************************************************
/* Base64 encode user:password                                   *
/*****************************************************************/
void create_basic_auth_strings() {
        basic_auth_influx = "";
        if (strcmp(USER_INFLUX, "") != 0 || strcmp(PWD_INFLUX, "") != 0) {
                basic_auth_influx = base64::encode(String(USER_INFLUX) + ":" + String(PWD_INFLUX));
        }
}

/*****************************************************************
/* start SDS011 sensor                                           *
/*****************************************************************/
void start_SDS() {
	debug_out(F("Start SDS"), DEBUG_INFO, 1);
	const uint8_t start_SDS_cmd[] = {0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0xAB};
	serialSDS.write(start_SDS_cmd, sizeof(start_SDS_cmd));
	is_SDS_running = true;
}

/*****************************************************************
/* stop SDS011 sensor                                            *
/*****************************************************************/
void stop_SDS() {
	debug_out(F("Stop SDS"), DEBUG_INFO, 1);
	const uint8_t stop_SDS_cmd[] = {0xAA, 0xB4, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB};
	serialSDS.write(stop_SDS_cmd, sizeof(stop_SDS_cmd));
	is_SDS_running = false;
}

/*****************************************************************
/* WiFi auto connecting script                                   *
/*****************************************************************/
void connect_WIFI() {
	int retry_count = 0;
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PWD); // Start WiFI

	debug_out(F("Connecting to "), DEBUG_INFO, 0);
	debug_out(WIFI_SSID, DEBUG_INFO, 1);

	while ((WiFi.status() != WL_CONNECTED) && (retry_count < WIFI_CONNECT_RETRIES)) {
		delay(500);
		debug_out(".", DEBUG_INFO, 0);
		retry_count++;
	}
	debug_out("", DEBUG_INFO, 1);
	if (WiFi.status() != WL_CONNECTED) {
		debug_out(F("Could not connect to WIFI"), DEBUG_ERROR, 1);
		return;
	}
	debug_out(F("WiFi connected\nIP address: "), DEBUG_INFO, 0);
	debug_out(ip_to_string(WiFi.localIP()), DEBUG_INFO, 1);
}

/*****************************************************************
/* send data to rest api                                         *
/*****************************************************************/
void send_data(const String& data, const int pin, const char* host, const int httpPort, const char* url, const char* basic_auth_string, const String& contentType) {
	debug_out(F("Start connecting to "), DEBUG_INFO, 0);
	debug_out(host, DEBUG_INFO, 1);

	String request_head = F("POST "); request_head += String(url); request_head += F(" HTTP/1.1\r\n");
	request_head += F("Host: "); request_head += String(host) + "\r\n";
	request_head += F("Content-Type: "); request_head += contentType + "\r\n";
	if (basic_auth_string != "") { request_head += F("Authorization: Basic "); request_head += String(basic_auth_string) + "\r\n";}
	request_head += F("X-PIN: "); request_head += String(pin) + "\r\n";
	request_head += F("X-Sensor: esp8266-"); request_head += esp_chip_id + "\r\n";
	request_head += F("Content-Length: "); request_head += String(data.length(), DEC) + "\r\n";
	request_head += F("Connection: close\r\n\r\n");

	if (httpPort == 443) {

		WiFiClientSecure client_s;

		client_s.setNoDelay(true);
		client_s.setTimeout(20000);

		if (!client_s.connect(host, httpPort)) {
			debug_out(F("connection failed"), DEBUG_ERROR, 1);
			return;
		}

		debug_out(F("Requesting URL: "), DEBUG_INFO, 0);
		debug_out(url, DEBUG_INFO, 1);

		client_s.print(request_head);
		client_s.println(data);

		delay(10);

		// Read reply from server and print them
		while(client_s.available()) {
			char c = client_s.read();
			debug_out(String(c), DEBUG_DEBUG, 0);
		}
	} else {

		WiFiClient client;

		client.setNoDelay(true);
		client.setTimeout(20000);

		if (!client.connect(host, httpPort)) {
			debug_out(F("connection failed"), DEBUG_ERROR, 1);
			return;
		}

		debug_out(F("Requesting URL: "), DEBUG_INFO, 0);
		debug_out(url, DEBUG_INFO, 1);

		client.print(request_head);
		client.println(data);

		delay(10);

		// Read reply from server and print them
		while(client.available()) {
			char c = client.read();
			debug_out(String(c), DEBUG_DEBUG, 0);
		}
	}

	debug_out(F("End connecting to "), DEBUG_INFO, 0);
	debug_out(host, DEBUG_INFO, 1);

	wdt_reset(); // nodemcu is alive
	yield();
}

/*****************************************************************
/* send single sensor data to luftdaten.info api                 *
/*****************************************************************/
void send_to_luftdaten(const String& data, const int pin, const char* host, const int httpPort, const char* url, const char* replace_str) {
  if (!SEND_TO_LUFTDATEN) {
    return;
	}

	String data_4_luftdaten = "";
	data_4_luftdaten  = data_first_part + data;
	data_4_luftdaten.remove(data_4_luftdaten.length() - 1);
	data_4_luftdaten.replace(replace_str, "");
	data_4_luftdaten += "]}";
	if (data != "") {
		send_data(data_4_luftdaten, pin, host, httpPort, url, "", FPSTR(TXT_CONTENT_TYPE_JSON));
	} else {
		debug_out(F("No data sent..."), DEBUG_INFO, 1);
	}
}

/*****************************************************************
/* send data to influxdb                                         *
/*****************************************************************/
String create_influxdb_string(const String& data) {
	String tmp_str;
	String data_4_influxdb;
	debug_out(F("Parse JSON for influx DB"), DEBUG_INFO, 1);
	debug_out(data, DEBUG_INFO, 1);
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject& json2data = jsonBuffer.parseObject(data);
	if (json2data.success()) {
		data_4_influxdb = "";
		data_4_influxdb += F("feinstaub,node=esp8266-");
		data_4_influxdb += esp_chip_id + " ";
		for (int i = 0; i < json2data["sensordatavalues"].size() - 1; i++) {
			tmp_str = jsonBuffer.strdup(json2data["sensordatavalues"][i]["value_type"].as<char*>());
			data_4_influxdb += tmp_str + "=";
			tmp_str = jsonBuffer.strdup(json2data["sensordatavalues"][i]["value"].as<char*>());
			data_4_influxdb += tmp_str;
			if (i < (json2data["sensordatavalues"].size() - 2)) { data_4_influxdb += ","; }
		}
		data_4_influxdb += "\n";
	} else {
		debug_out(F("Data read failed"), DEBUG_ERROR, 1);
	}
	return data_4_influxdb;
}

/*****************************************************************
/* send data as csv to serial out                                *
/*****************************************************************/
void send_csv(const String& data) {
	char* s;
	String tmp_str;
	String headline;
	String valueline;
	int value_count = 0;
	StaticJsonBuffer<1000> jsonBuffer;
	JsonObject& json2data = jsonBuffer.parseObject(data);
	debug_out(F("CSV Output"), DEBUG_INFO, 1);
	debug_out(data, DEBUG_INFO, 1);
	if (json2data.success()) {
		headline = F("Timestamp_ms;");
		valueline = String(current_time_ms) + ";";
		for (int i = 0; i < json2data["sensordatavalues"].size(); i++) {
			tmp_str = jsonBuffer.strdup(json2data["sensordatavalues"][i]["value_type"].as<char*>());
			headline += tmp_str + ";";
			tmp_str = jsonBuffer.strdup(json2data["sensordatavalues"][i]["value"].as<char*>());
			valueline += tmp_str + ";";
		}
		if (first_csv_line) {
			if (headline.length() > 0) { headline.remove(headline.length() - 1); }
			Serial.println(headline);
			first_csv_line = false;
		}
		if (valueline.length() > 0) { valueline.remove(valueline.length() - 1); }
		Serial.println(valueline);
	} else {
		debug_out(F("Data read failed"), DEBUG_ERROR, 1);
	}
}

/*****************************************************************
/* read DHT22 sensor values                                      *
/*****************************************************************/
String read_DHT() {
	String s = "";
	int i = 0;
	float h;
	float t;

	debug_out(F("Start reading DHT11/22"), DEBUG_DEBUG, 1);

	// Check if valid number if non NaN (not a number) will be send.

	last_value_DHT_T = "";
	last_value_DHT_H = "";

	while ((i++ < 5) && (s == "")) {
		h = dht.readHumidity(); //Read Humidity
		t = dht.readTemperature(); //Read Temperature
		if (isnan(t) || isnan(h)) {
			delay(100);
			h = dht.readHumidity(true); //Read Humidity
			t = dht.readTemperature(false, true); //Read Temperature
		}
		if (isnan(t) || isnan(h)) {
			debug_out(F("DHT22 couldn't be read"), DEBUG_ERROR, 1);
		} else {
			debug_out(F("Humidity    : "), DEBUG_INFO, 0);
			debug_out(String(h) + "%", DEBUG_INFO, 1);
			debug_out(F("Temperature : "), DEBUG_INFO, 0);
			debug_out(String(t) + char(223) + "C", DEBUG_INFO, 1);
			last_value_DHT_T = float_to_string(t);
			last_value_DHT_H = float_to_string(h);
			s += value_to_json(F("temperature"), last_value_DHT_T);
			s += value_to_json(F("humidity"), last_value_DHT_H);
			last_value_DHT_T.remove(last_value_DHT_T.length() - 1);
			last_value_DHT_H.remove(last_value_DHT_H.length() - 1);
		}
	}
	debug_out(F("------"), DEBUG_INFO, 1);

	debug_out(F("End reading DHT11/22"), DEBUG_DEBUG, 1);

	return s;
}

/*****************************************************************
/* read SDS011 sensor values                                     *
/*****************************************************************/
String read_SDS() {
	String s = "";
	String value_hex;
	char buffer;
	int value;
	int len = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is;
	int checksum_ok = 0;
	int position = 0;

	if (long(current_time_ms - start_time_ms) < (long(SEND_INTERVAL_MS) - long(SDS_WARMUP_TIME_MS + SDS_READING_TIME_MS))) {
		if (is_SDS_running) {
			stop_SDS();
		}
	} else {
		debug_out(F("Start reading SDS"), DEBUG_DEBUG, 1);
		if (!is_SDS_running) {
			start_SDS();
		}

		while (serialSDS.available() > 0) {
			buffer = serialSDS.read();
			debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_DEBUG, 1);
			value = int(buffer);
			switch (len) {
			case (0): if (value != 170) { len = -1; }; break;
			case (1): if (value != 192) { len = -1; }; break;
			case (2): pm25_serial = value; checksum_is = value; break;
			case (3): pm25_serial += (value << 8); checksum_is += value; break;
			case (4): pm10_serial = value; checksum_is += value; break;
			case (5): pm10_serial += (value << 8); checksum_is += value; break;
			case (6): checksum_is += value; break;
			case (7): checksum_is += value; break;
			case (8):
				debug_out(F("Checksum is: "), DEBUG_DEBUG, 0); debug_out(String(checksum_is % 256), DEBUG_DEBUG, 0);
				debug_out(F(" - should: "), DEBUG_DEBUG, 0); debug_out(String(value), DEBUG_DEBUG, 1);
				if (value == (checksum_is % 256)) { checksum_ok = 1; } else { len = -1; }; break;
			case (9): if (value != 171) { len = -1; }; break;
			}
			len++;
			if (len == 10 && checksum_ok == 1 && (long(current_time_ms - start_time_ms) > (long(SEND_INTERVAL_MS) - long(SDS_READING_TIME_MS)))) {
				if ((! isnan(pm10_serial)) && (! isnan(pm25_serial))) {
					sds_pm10_sum += pm10_serial;
					sds_pm25_sum += pm25_serial;
					if (sds_pm10_min > pm10_serial) { sds_pm10_min = pm10_serial; }
					if (sds_pm10_max < pm10_serial) { sds_pm10_max = pm10_serial; }
					if (sds_pm25_min > pm25_serial) { sds_pm25_min = pm25_serial; }
					if (sds_pm25_max < pm25_serial) { sds_pm25_max = pm25_serial; }
					debug_out(F("PM10 (sec.) : "), DEBUG_DEBUG, 0); debug_out(float_to_string(float(pm10_serial) / 10), DEBUG_DEBUG, 1);
					debug_out(F("PM2.5 (sec.): "), DEBUG_DEBUG, 0); debug_out(float_to_string(float(pm25_serial) / 10), DEBUG_DEBUG, 1);
					sds_val_count++;
				}
				len = 0; checksum_ok = 0; pm10_serial = 0.0; pm25_serial = 0.0; checksum_is = 0;
			}
			yield();
		}

	}
	if (send_now) {
		last_value_SDS_P1 = "";
		last_value_SDS_P2 = "";
		if (sds_val_count > 2) {
			sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
			sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
			sds_val_count = sds_val_count - 2;
		}
		if (sds_val_count > 0) {
			debug_out("PM10:  " + float_to_string(float(sds_pm10_sum) / (sds_val_count * 10.0)), DEBUG_INFO, 1);
			debug_out("PM2.5: " + float_to_string(float(sds_pm25_sum) / (sds_val_count * 10.0)), DEBUG_INFO, 1);
			debug_out("------", DEBUG_INFO, 1);
			last_value_SDS_P1 = float_to_string(float(sds_pm10_sum) / (sds_val_count * 10.0));
			last_value_SDS_P2 = float_to_string(float(sds_pm25_sum) / (sds_val_count * 10.0));
			s += value_to_json("SDS_P1", last_value_SDS_P1);
			s += value_to_json("SDS_P2", last_value_SDS_P2);
			last_value_SDS_P1.remove(last_value_SDS_P1.length() - 1);
			last_value_SDS_P2.remove(last_value_SDS_P2.length() - 1);
		}
		sds_pm10_sum = 0; sds_pm25_sum = 0; sds_val_count = 0;
		sds_pm10_max = 0; sds_pm10_min = 20000; sds_pm25_max = 0; sds_pm25_min = 20000;
		if ((SEND_INTERVAL_MS > (SDS_WARMUP_TIME_MS + SDS_READING_TIME_MS))) {
			stop_SDS();
		}
	}

	return s;
}

/*****************************************************************
/* The Setup                                                     *
/*****************************************************************/
void setup() {
	Serial.begin(9600);
	esp_chip_id = String(ESP.getChipId());
	WiFi.persistent(false);
	connect_WIFI();
	serialSDS.begin(9600);
	dht.begin();	// Start DHT
	delay(10);
	debug_out(F("Chip id: "), DEBUG_INFO, 0);
	debug_out(esp_chip_id, DEBUG_INFO, 1);
	create_basic_auth_strings();

  debug_out(F("Configuration:"), DEBUG_INFO, 1);
	if (SDS_ENABLED) { debug_out(F(" * Read SDS"), DEBUG_INFO, 1); }
	if (DHT_ENABLED) { debug_out(F(" * Read DHT"), DEBUG_INFO, 1); }
	if (SEND_TO_LUFTDATEN) { debug_out(F(" * Send to luftdaten.info"), DEBUG_INFO, 1); }
	if (SEND_TO_MADAVI) { debug_out(F(" * Send to madavi.de"), DEBUG_INFO, 1); }
	if (SEND_TO_OPENSENSEMAP) { debug_out(F(" * Send to opensensemap.org"), DEBUG_INFO, 1); }
	if (SEND_TO_CSV) { debug_out(F(" * Send to CSV to serial"), DEBUG_INFO, 1); }
	if (SEND_TO_INFLUX) { debug_out(F(" * Send to custom influx DB"), DEBUG_INFO, 1); }

	if (SDS_ENABLED) {
		stop_SDS();
	}

  wdt_disable();
	wdt_enable(WATHDOG_TIMER_MS);

	start_time_ms = millis();
	start_SDS_time_ms = millis();
}

/*****************************************************************
/* And action                                                    *
/*****************************************************************/
void loop() {
	String data = "";
	String data_4_influxdb = "";
	String data_sample_times = "";

	String result_SDS = "";
	String result_DHT = "";
	String signal_strength = "";

	unsigned long sum_send_time_ms = 0;
	unsigned long start_send_time_ms;

	send_failed = false;

	act_micro = micros();
	current_time_ms = millis();
	send_now = (current_time_ms - start_time_ms) > SEND_INTERVAL_MS;

	sample_count++;

	wdt_reset(); // nodemcu is alive

	if (last_micro != 0) {
		diff_micro = act_micro - last_micro;
		if (max_micro < diff_micro) { max_micro = diff_micro;}
		if (min_micro > diff_micro) { min_micro = diff_micro;}
		last_micro = act_micro;
	} else {
		last_micro = act_micro;
	}

	if (((current_time_ms - start_SDS_time_ms) > SDS_SAMPLE_TIME_MS) || ((current_time_ms - start_time_ms) > SEND_INTERVAL_MS)) {
		if (SDS_ENABLED) {
			result_SDS = read_SDS();
			start_SDS_time_ms = current_time_ms;
		}
	}

	if (send_now) {
		if (DHT_ENABLED) {
			debug_out(F("Call read_DHT"), DEBUG_DEBUG, 1);
			result_DHT = read_DHT();
		}

		data = data_first_part;
		data_sample_times  = value_to_json("samples", String(long(sample_count)));
		data_sample_times += value_to_json("min_micro", String(long(min_micro)));
		data_sample_times += value_to_json("max_micro", String(long(max_micro)));

		signal_strength = String(WiFi.RSSI());

		if (SDS_ENABLED) {
			data += result_SDS;
			if (SEND_TO_LUFTDATEN) {
				debug_out(F("## Sending to luftdaten.info (SDS): "), DEBUG_INFO, 1);
				start_send_time_ms = millis();
				send_to_luftdaten(result_SDS, SDS_API_PIN, HOST_LUFTDATEN, PORT_LUFTDATEN, URL_LUFTDATEN, "SDS_");
				sum_send_time_ms += millis() - start_send_time_ms;
			}
		}

		if (DHT_ENABLED) {
			data += result_DHT;
			if (SEND_TO_LUFTDATEN) {
				debug_out(F("## Sending to luftdaten.info (DHT): "), DEBUG_INFO, 1);
				start_send_time_ms = millis();
				send_to_luftdaten(result_DHT, DHT_API_PIN, HOST_LUFTDATEN, PORT_LUFTDATEN, URL_LUFTDATEN, "DHT_");
				sum_send_time_ms += millis() - start_send_time_ms;
			}
		}

		data_sample_times += value_to_json("signal", signal_strength);
		data += data_sample_times;

		if (data.lastIndexOf(',') == (data.length() - 1)) {
			data.remove(data.length() - 1);
		}
		data += "]}";
		debug_out(data, DEBUG_INFO, 1);

		// sending to api(s)

		if (SEND_TO_MADAVI) {
			debug_out(F("## Sending to madavi.de: "), DEBUG_INFO, 1);
			start_send_time_ms = millis();
			send_data(data, 0, HOST_MADAVI, PORT_MADAVI, URL_MADAVI, "", FPSTR(TXT_CONTENT_TYPE_JSON));
			sum_send_time_ms += millis() - start_send_time_ms;
		}

		if (SEND_TO_OPENSENSEMAP) {
			debug_out(F("## Sending to opensensemap: "), DEBUG_INFO, 1);
			start_send_time_ms = millis();
			send_data(data, 0, HOST_OPENSENSEMAP, PORT_OPENSENSEMAP, URL_OPENSENSEMAP, "", FPSTR(TXT_CONTENT_TYPE_JSON));
			sum_send_time_ms += millis() - start_send_time_ms;
		}

		if (SEND_TO_INFLUX) {
			debug_out(F("## Sending to custom influx db: "), DEBUG_INFO, 1);
			data_4_influxdb = create_influxdb_string(data);
			start_send_time_ms = millis();
			send_data(data_4_influxdb, 0, HOST_INFLUX, PORT_INFLUX, URL_INFLUX, basic_auth_influx.c_str(), FPSTR(TXT_CONTENT_TYPE_INFLUXDB));
			sum_send_time_ms += millis() - start_send_time_ms;
		}

		if (SEND_TO_CSV) {
			debug_out(F("## Sending as csv: "), DEBUG_INFO, 1);
			send_csv(data);
		}

		debug_out(F("Time (ms) for sending data: "), DEBUG_INFO, 0);
		debug_out(String(sum_send_time_ms), DEBUG_INFO, 1);

		if (WiFi.status() != WL_CONNECTED) {  // reconnect if connection lost
			connect_WIFI();
		}

		// Reset variables for next sampling
		sample_count = 0;
		last_micro = 0;
		min_micro = 1000000000;
		max_micro = 0;
		start_time_ms = millis(); // store the start time
	}
	yield();
}
