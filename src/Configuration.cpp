#include "Configuration.h"

#include <LittleFS.h>

Configuration::Configuration() {
	// File paths to save input values permanently
	ssidPath = "/ssid.txt";
	passPath = "/pass.txt";
	hostnamePath = "/hostname.txt";

	fsInitialized = false;
}

void Configuration::initLittleFS() {
	if (fsInitialized) {
		return;
	}

	fsInitialized = true;

	if (!LittleFS.begin()) {
		Serial.println("An error has occurred while mounting LittleFS");
		return;
	}

	Serial.println("LittleFS mounted successfully");
}

void Configuration::writeSSID(const char *ssid) {
	writeFile(LittleFS, ssidPath, ssid);
}

void Configuration::writePass(const char *pass) {
	writeFile(LittleFS, passPath, pass);
}

void Configuration::writeHostname(const char *hostname) {
	writeFile(LittleFS, hostnamePath, hostname);
}

String Configuration::getSSID() {
	return readFile(LittleFS, ssidPath);
}

String Configuration::getPass() {
	return readFile(LittleFS, passPath);
}

String Configuration::getHostname() {
	return readFile(LittleFS, hostnamePath);
}

// Read File from LittleFS
String Configuration::readFile(fs::FS &fs, const char *path) {
	initLittleFS();

	Serial.printf("Reading file: %s\r\n", path);

	File file = fs.open(path, "r");
	if (!file || file.isDirectory()) {
		Serial.println("- failed to open file for reading");
		return String();
	}

	String fileContent;
	while (file.available()) {
		fileContent = file.readStringUntil('\n');
		break;
	}

	return fileContent;
}

// Write file to LittleFS
void Configuration::writeFile(fs::FS &fs, const char *path, const char *message) {
	initLittleFS();

	Serial.printf("Writing file: %s\r\n", path);

	File file = fs.open(path, "w");
	if (!file) {
		Serial.println("- failed to open file for writing");
		return;
	}
	if (file.print(message)) {
		Serial.println("- file written");
	} else {
		Serial.println("- write failed");
	}
}
