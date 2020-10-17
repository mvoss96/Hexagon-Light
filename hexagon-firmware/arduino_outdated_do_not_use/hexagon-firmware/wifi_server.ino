#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <SPIFFS.h>
#include <FS.h>
#include <string>

using namespace httpsserver;
extern bool power;
extern int WW, CW;
unsigned int brightness;
unsigned int color;
char contentTypes[][2][32] = {
  {".html", "text/html"},
  {".css",  "text/css"},
  {".js",   "application/javascript"},
  {".json", "application/json"},
  {".png",  "image/png"},
  {".jpg",  "image/jpg"},
  {".svg",  "image/svg+xml"}
};

//SSLCert * cert;
HTTPServer * secureServer;
void handleSPIFFS(HTTPRequest * req, HTTPResponse * res);

unsigned int calcColor() {
  if (WW + CW == 0)
    return 0;
  return ((int)(100 * ((float)WW / (WW + CW))));
}

unsigned int calcBrightness() {
  if (WW + CW == 0)
    return 0;
  return (int)(100 * ((float)(WW + CW) / 255));
}

void setLED() {
  if (color == 0) {
    WW = 0;
    CW = 0;
  }
  WW = ((brightness / 100.0) * 255) * (color / 100.0);
  CW = ((brightness / 100.0) * 255) * (1 - (color / 100.0));
#ifdef DEBUG
  Serial.print("calculating br: ");
  Serial.print(brightness);
  Serial.print(" color: ");
  Serial.println(color);
  Serial.print("WW: ");
  Serial.print(WW);
  Serial.print(" CW: ");
  Serial.println(CW);
#endif
}

void serverSetup() {
  brightness = calcBrightness();
  color = calcColor();
  Serial.println("starting webserver...");
  Serial.println("mounting SPIFFS...");
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
    return;
  }
  secureServer = new HTTPServer();

  ResourceNode * nodePower = new ResourceNode("/power", "GET", [](HTTPRequest * req, HTTPResponse * res) {
    res->setHeader("Content-Type", "text/plain");
    res->print((power) ? "ON" : "OFF");
  });
  ResourceNode * nodePowerPost = new ResourceNode("/power", "POST", [](HTTPRequest * req, HTTPResponse * res) {
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    if (buffer[0] == 'O' && buffer[1] == 'F' && buffer[2] == 'F')
      power = false;
    else if (buffer[0] == 'O' && buffer[1] == 'N')
      power = true;
    res->setHeader("Content-Type", "text/plain");
    res->print("");
  });

  ResourceNode * nodeColor = new ResourceNode("/color", "GET", [](HTTPRequest * req, HTTPResponse * res) {
   #ifdef DEBUG
   Serial.print("GET Brightness: ");
   Serial.println(color);
   #endif
    res->setHeader("Content-Type", "text/plain");
    char msg[4];
    sprintf(msg,"%2d",color);
    res->print(msg);
  });
  ResourceNode * nodeColorPost = new ResourceNode("/color", "POST", [](HTTPRequest * req, HTTPResponse * res) {
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    color = atoi((char*)buffer);
    setLED();
#ifdef DEBUG
    Serial.print("received: ");
    Serial.println(color);
#endif
    res->setHeader("Content-Type", "text/plain");
    res->print("");
  });

  ResourceNode * nodeBr = new ResourceNode("/brightness", "GET", [](HTTPRequest * req, HTTPResponse * res) {
    #ifdef DEBUG
    Serial.print("GET Brightness: ");
    Serial.println(brightness);
    #endif
    res->setHeader("Content-Type", "text/plain");
    char msg[4];
    sprintf(msg,"%2d",brightness);
    res->print(msg);
  });
  ResourceNode * nodeBrPost = new ResourceNode("/brightness", "POST", [](HTTPRequest * req, HTTPResponse * res) {
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    brightness = atoi((char*)buffer);
    setLED();
#ifdef DEBUG
    Serial.print("received: ");
    Serial.println(brightness);
#endif
    res->setHeader("Content-Type", "text/plain");
    res->print("");
  });

  ResourceNode * spiffsNode = new ResourceNode("", "", &handleSPIFFS);
  secureServer->setDefaultNode(spiffsNode);
  secureServer->registerNode(nodePower);
  secureServer->registerNode(nodePowerPost);
  secureServer->registerNode(nodeColor);
  secureServer->registerNode(nodeColorPost);
  secureServer->registerNode(nodeBr);
  secureServer->registerNode(nodeBrPost);

  Serial.println("Starting server...");
  secureServer->start();
  if (secureServer->isRunning()) {
    Serial.println("Server ready.");
  }
  else {
    Serial.println("Server could not be started");
    delay(100);
    ESP.restart();
  }
}


void serverRun() {
  secureServer->loop();
}

void handleSPIFFS(HTTPRequest * req, HTTPResponse * res) {

  // We only handle GET here
  if (req->getMethod() == "GET") {
    // Redirect / to /index.html
    std::string reqFile = req->getRequestString() == "/" ? "/index.html" : req->getRequestString();

    // Try to open the file
    std::string filename = reqFile;

    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str())) {
      Serial.print("not found: ");
      Serial.println(filename.c_str());
      // Send "404 Not Found" as response, as the file doesn't seem to exist
      res->setStatusCode(404);
      res->setStatusText("Not found");
      res->println("404 Not Found");
      return;
    }

    File file = SPIFFS.open(filename.c_str());

    // Set length
    res->setHeader("Content-Length", httpsserver::intToString(file.size()));

    // Content-Type is guessed using the definition of the contentTypes-table defined above
    int cTypeIdx = 0;
    do {
      if (reqFile.rfind(contentTypes[cTypeIdx][0]) != std::string::npos) {
        res->setHeader("Content-Type", contentTypes[cTypeIdx][1]);
        break;
      }
      cTypeIdx += 1;
    } while (strlen(contentTypes[cTypeIdx][0]) > 0);

    // Read the file and write it to the response
    uint8_t buffer[256];
    size_t length = 0;
    do {
      length = file.read(buffer, 256);
      res->write(buffer, length);
    } while (length > 0);

    file.close();
  } else {
    // If there's any body, discard it
    req->discardRequestBody();
    // Send "405 Method not allowed" as response
    res->setStatusCode(405);
    res->setStatusText("Method not allowed");
    res->println("405 Method not allowed");
  }
}
