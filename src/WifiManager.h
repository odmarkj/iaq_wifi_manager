#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#include "Configuration.h"

class WifiManagerClass {
	public:
		WifiManagerClass();

		bool connectToWifi();

		void startManagementServer();
		void startManagementServer(const char *ssid);

		void check();

		String getHostname();
		String getSSID();
		int8_t getRSSI();
		IPAddress getIP();

		bool isConnected();

	private:
		DNSServer dnsServer;
		AsyncWebServer _server;
		Configuration _config;

		bool _connected;

		int _reconnectIntervalCheck;
		int _connectionTimeout;

		String _networks;
		String _hostname;
		String _ssid;

		IPAddress _ip;

		unsigned long _nextReconnectCheck;

		String getAvailableNetworks();

		bool waitForConnection();
		void serveDefaultUI();
		bool acceptsCompressedResponse(AsyncWebServerRequest *request);
};

extern WifiManagerClass WifiManager;

#endif
