#include "WifiManager.h"

#include <ESPAsyncWebServer.h>
#include <LEAmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "Data.h"
#include "Configuration.h"

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

WifiManagerClass::WifiManagerClass()
	: _server(80), _config() {
	_reconnectIntervalCheck = 5000;
	_connectionTimeout = 10000;

	_nextReconnectCheck = 0;
	_connected = false;

	_networks = "";
	_hostname = "";
	_ssid = "";
}

void WifiManagerClass::check() {
	dnsServer.processNextRequest();
	MDNS.update();

	if (_connected && millis() > _nextReconnectCheck) {
		if (WiFi.status() != WL_CONNECTED) {
			_connected = false;

			Serial.println("WiFi connection lost. Attempting to reconnect.");

			// FIXME
			//WiFi.reconnect();
			WiFi.disconnect();

			waitForConnection();
		}

		_nextReconnectCheck = millis() + _reconnectIntervalCheck;
	}
}

String WifiManagerClass::getAvailableNetworks() {
	Serial.println("Scanning networks...");

	// We need to make sure we are disconnected
	// before trying to scan for networks. We do
	// not care about the return value from it.
	WiFi.disconnect();

	byte networks = WiFi.scanNetworks();

	String json = "[";
	String separator = "";

	// If negative value is returned from the scan we will
	// just return the empty list as the loop will not
	// run. This is in case of WIFI_SCAN_FAILED or similar.
	for (int i = 0; i < networks; i++) {
		String network = String("\"") + WiFi.SSID(i) + "\"";

		if (json.indexOf(network) == -1) {
			json += separator + network;
			separator = ",";
		}
	}

	json += "]";

	return json;
}

bool WifiManagerClass::connectToWifi() {
	_ssid = _config.getSSID();
	_hostname = _config.getHostname();
	String pass = _config.getPass();

	if (_ssid == "") {
		Serial.println("No connection information specified");

		return false;
	}

	WiFi.mode(WIFI_STA);
	// FIXME
	//WiFi.setSleep(WIFI_PS_NONE);

	// Fixes issue with mDNS where hostname was not set (v1.0.1) and mDNS crashed (v1.0.2)
	//WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

	if (_hostname != "") {
		WiFi.setHostname(_hostname.c_str());
		Serial.println("Setting hostname " + _hostname);

		if (MDNS.begin(_hostname.c_str())) {
			Serial.println("mDNS responder started");
		} else {
			Serial.println("Unable to start mDNS responder");
		}
	} else {
		Serial.println("No hostname configured");
	}

	Serial.println("Connecting to WiFi...");

	Serial.println(_ssid.c_str());
	Serial.println(pass.c_str());

	WiFi.begin(_ssid.c_str(), pass.c_str());

	_connected = waitForConnection();

	return _connected;
}

bool WifiManagerClass::waitForConnection() {
	unsigned long timeout = millis() + _connectionTimeout;

	while (WiFi.status() != WL_CONNECTED) {
		if (millis() > timeout) {
			Serial.println("Unable to connect to WIFI");

			return false;
		}
	}

	_ip = WiFi.localIP();

	Serial.println("Assigned IP Address:");
	Serial.println(_ip);


	return true;
}

void WifiManagerClass::startManagementServer() {
    startManagementServer("WIFI-MANAGER");
}

void WifiManagerClass::startManagementServer(const char *ssid) {
	Serial.println("Starting Management AP");

	// Prepare list of available networks
	_networks = getAvailableNetworks();

  	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(ssid);

	_ssid = WiFi.softAPSSID();
	_ip = WiFi.softAPIP();

	Serial.println("Server IP Address:");
	Serial.println(_ip);

	dnsServer.setTTL(3600);
	dnsServer.start(DNS_PORT, "*", _ip);

	bool hasCustomUI = LittleFS.exists("/wifi-manager/index.html");

	if (hasCustomUI) {
		
		_server
			.serveStatic("/", LittleFS, "/wifi-manager")
			.setDefaultFile("index.html");

		File file = LittleFS.open("/wifi-manager/index.html", "r");
		String indexHTML = file.readString();
		file.close();

		Serial.println("Serving custom UI from /wifi-manager");
		_server.onNotFound([=](AsyncWebServerRequest *request) {
			request->send(200, "text/html", indexHTML);
		});
	} else {
		serveDefaultUI();

		Serial.println("Serving default UI");
	}

	_server.on("/networks", HTTP_GET, [=](AsyncWebServerRequest *request) {
		request->send(200, "application/json", _networks);
	});

	_server.on("/credentials", HTTP_PUT, [=](AsyncWebServerRequest *request) {
		int params = request->params();

		for (int i = 0; i < params; i++) {
			AsyncWebParameter *param = request->getParam(i);

			if (param->isPost()) {
				if (param->name() == "ssid") {
					String ssid = param->value().c_str();
					_config.writeSSID(ssid.c_str());
				}

				if (param->name() == "password") {
					String pass = param->value().c_str();
					_config.writePass(pass.c_str());
				}

				if (param->name() == "hostname") {
					String hostname = param->value().c_str();
					_config.writeHostname(hostname.c_str());
				}
			}
		}

		request->send(204);

		delay(1000);

		//ESP.restart();
		rp2040.reboot();
	});

	_server.begin();
}

void WifiManagerClass::serveDefaultUI() {
    Serial.printf("Compressed html: %d bytes\n", gzipBytes);
    Serial.printf("Uncompressed html: %d bytes\n", htmlBytes);

	_server.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        bool useGzip = acceptsCompressedResponse(request);

        if (useGzip) {
            Serial.println("Serving gzipped response");

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", gzip, gzipBytes);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else {
            Serial.println("Serving uncompressed html");

            request->send_P(200, "text/html", html);
        }
	});

	_server.onNotFound([=](AsyncWebServerRequest *request) {
		request->redirect("/");
	});

}

bool WifiManagerClass::acceptsCompressedResponse(AsyncWebServerRequest *request) {
    if (request->hasHeader("Accept-Encoding")){
        AsyncWebHeader* header = request->getHeader("Accept-Encoding");
        String value = header->value();
        bool hasGzip = value.indexOf("gzip") > -1;

        return hasGzip;
    }

    return false;
}

String WifiManagerClass::getHostname() {
	return _hostname;
}

String WifiManagerClass::getSSID() {
	return _ssid;
}

int8_t WifiManagerClass::getRSSI() {
	return WiFi.RSSI();
}

IPAddress WifiManagerClass::getIP() {
	return _ip;
}

bool WifiManagerClass::isConnected() {
	return _connected;
}

WifiManagerClass WifiManager;
