#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "Configuration.h"

class WifiManager {
	private:
		AsyncWebServer _server;
		Configuration _config;

		bool _allowReboot;
		bool _connected;

		int _reconnectIntervalCheck;
		int _connectionTimeout;

		String _networks;

		unsigned long _nextReconnectCheck;

		String getAvailableNetworks();

		bool waitForConnection();
		void serveDefaultUI();
		bool acceptsCompressedResponse(AsyncWebServerRequest *request);

	public:
		WifiManager();

		bool connectToWifi();

		void allowReboot();
		void allowReboot(bool allow);

		void startManagementServer();
		void startManagementServer(const char *ssid);

		void check();
};

#endif
