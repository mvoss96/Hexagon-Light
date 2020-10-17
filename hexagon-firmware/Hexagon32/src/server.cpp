#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <SPIFFS.h>
#include <FS.h>
#include <WiFi.h>

using namespace httpsserver;
extern bool power;
extern uint8_t brightness;
extern uint8_t color;
char contentTypes[][2][32] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpg"},
    {".svg", "image/svg+xml"}};

//SSLCert * cert;
HTTPServer *secureServer;
void handleSPIFFS(HTTPRequest *req, HTTPResponse *res);

void spiffsSetup()
{
  Serial.println("mounting SPIFFS...");
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {

    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void serverSetup()
{
  Serial.println("starting webserver...");
  secureServer = new HTTPServer();

  ResourceNode *nodePower = new ResourceNode("/power", "GET", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    res->print((power) ? "on" : "off");
  });

  ResourceNode *nodeColor = new ResourceNode("/color", "GET", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    char msg[4];
    sprintf(msg, "%2d", color);
    res->print(msg);
  });

  ResourceNode *nodeColorMired = new ResourceNode("/mired-color", "GET", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    char msg[4];
    sprintf(msg, "%2d", (int)(50+3.5*color));
    res->print(msg);
  });

  ResourceNode *nodeBr = new ResourceNode("/brightness", "GET", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    char msg[4];
    sprintf(msg, "%2d", brightness);
    res->print(msg);
  });

  ResourceNode *nodeIp = new ResourceNode("/ip", "GET", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    res->print(WiFi.localIP().toString());
  });

  ResourceNode *nodePowerPost = new ResourceNode("/power", "POST", [](HTTPRequest *req, HTTPResponse *res) {
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    if (memcmp(buffer, "off", 3) == 0 || memcmp(buffer, "OFF", 3) == 0)
    {
      power = false;
      res->setStatusCode(200);
    }
    else if (memcmp(buffer, "on", 2) == 0 || memcmp(buffer, "ON", 2) == 0)
    {
      power = true;
      res->setStatusCode(200);
    }
    else
    {
      res->setStatusCode(400);
    }
    res->setHeader("Content-Type", "text/plain");
    res->print("");
  });

  ResourceNode *nodeColorPost = new ResourceNode("/color", "POST", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    int val = atoi((char *)buffer);
    if (val >= 0 && val <= 100)
    {
      color = val;
      res->setStatusCode(200);
    }
    else
    {
      res->setStatusCode(400);
    }
    res->print("");
  });

  ResourceNode *nodeColorMiredPost = new ResourceNode("/mired-color", "POST", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
    byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    int val = atoi((char *)buffer);
    if (val >= 0 && val <= 400)
    {
      color = (int)((val-50)/3.5);
      res->setStatusCode(200);
    }
    else
    {
      res->setStatusCode(400);
    }
    res->print("");
  });

  ResourceNode *nodeBrPost = new ResourceNode("/brightness", "POST", [](HTTPRequest *req, HTTPResponse *res) {
    res->setHeader("Content-Type", "text/plain");
     byte buffer[5];
    while (!(req->requestComplete()))
      size_t s = req->readBytes(buffer, 5);
    int val = atoi((char *)buffer);
    if (val >= 0 && val <= 255)
    {
      brightness = val;
      res->setStatusCode(200);
    }
    else
    {
      res->setStatusCode(400);
    }
    res->print("");
  });

  ResourceNode *spiffsNode = new ResourceNode("", "", &handleSPIFFS);
  secureServer->setDefaultNode(spiffsNode);
  secureServer->registerNode(nodePower);
  secureServer->registerNode(nodePowerPost);
  secureServer->registerNode(nodeColor);
  secureServer->registerNode(nodeColorMired);
  secureServer->registerNode(nodeColorMiredPost);
  secureServer->registerNode(nodeColorPost);
  secureServer->registerNode(nodeBr);
  secureServer->registerNode(nodeBrPost);
  secureServer->registerNode(nodeIp);

  Serial.println("Starting server...");
  secureServer->start();
  if (secureServer->isRunning())
  {
    Serial.println("Server ready.");
  }
  else
  {
    Serial.println("Server could not be started");
    delay(100);
    ESP.restart();
  }
}

void serverRun()
{
  secureServer->loop();
}

void handleSPIFFS(HTTPRequest *req, HTTPResponse *res)
{

  // We only handle GET here
  if (req->getMethod() == "GET")
  {
    // Redirect / to /index.html
    std::string reqFile = req->getRequestString() == "/" ? "/index.html" : req->getRequestString();

    // Try to open the file
    std::string filename = reqFile;

    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str()))
    {
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
    do
    {
      if (reqFile.rfind(contentTypes[cTypeIdx][0]) != std::string::npos)
      {
        res->setHeader("Content-Type", contentTypes[cTypeIdx][1]);
        break;
      }
      cTypeIdx += 1;
    } while (strlen(contentTypes[cTypeIdx][0]) > 0);

    // Read the file and write it to the response
    uint8_t buffer[256];
    size_t length = 0;
    do
    {
      length = file.read(buffer, 256);
      res->write(buffer, length);
    } while (length > 0);

    file.close();
  }
  else
  {
    // If there's any body, discard it
    req->discardRequestBody();
    // Send "405 Method not allowed" as response
    res->setStatusCode(405);
    res->setStatusText("Method not allowed");
    res->println("405 Method not allowed");
  }
}