#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <LittleFS.h>

class Configuration {
protected:
  void initLittleFS();

  void writeFile(fs::FS &fs, const char *path, const char *message);
  String readFile(fs::FS &fs, const char *path);

  const char *ssidPath;
  const char *passPath;
  const char *hostnamePath;

  bool fsInitialized;

public:
  Configuration();

  void writeSSID(const char *ssid);
  void writePass(const char *pass);
  void writeHostname(const char *hostname);

  String getSSID();
  String getPass();
  String getHostname();
};

#endif
