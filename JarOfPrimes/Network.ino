/* Jar of Primes - Network
 *  
 * This sketch contains network-support for the Jar of Primes.
 *
 * Marian Aldenhövel <marian.aldenhoevel@marian-aldenhoevel.de>
 * Published under the MIT license. See license.html or 
 * https://opensource.org/licenses/MIT
*/

const String MIMETYPE_JSON = "application/json";
const long STA_CONNECTION_TIMEOUT = 10*1000;

String hostName;

// Global variable for the servers.
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const byte DNS_PORT = 53;
DNSServer dnsServer;

// Setup for captive portal. 
IPAddress softAP_IP(192, 168, 4, 1);
IPAddress softAP_NetMask(255, 255, 255, 0);

String WiFiMode = "";

void setupAsAP() {
  // AP-SSID or passphrase are not set, so create an AP users can connect to and
  // configure the system.
  digitalWrite(LED_PIN_ESP, LOW);
  delay(500);
  digitalWrite(LED_PIN_ESP, HIGH);
  delay(500);
  digitalWrite(LED_PIN_ESP, LOW);

  Serial.printf("Configuring as access point SSID \"%s\"...\n", hostName.c_str());

  WiFiMode = "WIFI_AP";  
  WiFi.mode(WIFI_AP);
  delay(100);

  WiFi.softAPConfig(softAP_IP, softAP_IP, softAP_NetMask);
  if (!WiFi.softAP(hostName.c_str(), "")) {
    Serial.printf("WiFi.softAP() failed.\n");
  }

  Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());

  Serial.printf("Setting up DNS server for captive portal...\n");
  /* Setup the DNS server redirecting all the domains to the softAP_IP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", softAP_IP);  
  digitalWrite(LED_PIN_ESP, HIGH);
}

void setupAsSTA() {
  Serial.printf("Connecting to network \"%s\" with passphrase \"%s\"...\n", ssid.c_str(), passphrase.c_str());

  WiFiMode = "WIFI_STA";
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), passphrase.c_str());
  delay(100);
  
  // Wait for connection, blink LED while doing so.
  int led = LOW;
  long until = millis() + STA_CONNECTION_TIMEOUT;
  while ((WiFi.status() != WL_CONNECTED) && (millis() <= until)) {
    digitalWrite(LED_PIN_ESP, led);
    if (led == HIGH) {
      led = LOW;
    } else {
      led = HIGH;
    }
    delay(500);
    
    Serial.printf(".");
  }
  Serial.printf("\n");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected to network \"%s\". IP address: %s\n", ssid.c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.printf("Connection to network \"%s\" timed out.\n", ssid.c_str());
  } 
  
  digitalWrite(LED_PIN_ESP, HIGH);
}

String methodToString(HTTPMethod method) {
  switch (method) {
    case HTTP_GET: return "GET";
    case HTTP_POST: return "POST";
    case HTTP_DELETE: return "DELETE";
    case HTTP_OPTIONS: return "OPTIONS";
    case HTTP_PUT: return "PUT";
    case HTTP_PATCH: return "PATCH";
    default: return String(method);
  }
}

// See: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/src/detail/RequestHandler.h
class StaticFileWithLEDHandler: public RequestHandler {

  String getFilenameFromUri(String uri) {
    return uri; // TODO: Strip query-params etc.
  }
  
  bool canHandle(HTTPMethod method, String uri) {
    String filename = getFilenameFromUri(uri);
    return (SPIFFS.exists(filename +  ".gz") || SPIFFS.exists(filename));
  }

  bool handle(ESP8266WebServer &server, HTTPMethod method, String uri) {   
    digitalWrite(LED_PIN_ESP, LOW);

    logRequest();
    
    bool result = false;
    String filename = uri; // TODO: Strip query-params etc.
 
    File content;
    if (SPIFFS.exists(filename + ".gz")) {
      Serial.printf("(serving .gz) ");
      content = SPIFFS.open(filename + ".gz", "r");
    } else {
      content = SPIFFS.open(filename, "r");
    }

    if (content) { 
      // Create a cache-tag for the file. We do not have timestamps on
      // SPIFFS, so we use the size of the file instead.
      String etag = "\"" + String(content.size()) + "\"";
      server.sendHeader("Cache-Control", "max-age=3600");
      server.sendHeader("ETag", etag);

      // Has the client provided If-None-Match?
      if (server.hasHeader("If-None-Match")) {
        // Yes. And is it the same as we think it should be?
        String clientEtag = server.header("If-None-Match");
        if (etag == clientEtag) {
          // Yes. Don't send content, but a 304 status.
          server.send(304, "text/plain", "304 Not modified");
          Serial.printf("sent not modified.\n");
          result = true;
        }
      }

      // Handled yet? Maybe with a 304?
      if (!result) {
        // No. Serve the file.
        String contentType = getContentType(filename);
        
        size_t sent = server.streamFile(content, contentType);
        Serial.printf("%s sent %d bytes.\n", contentType.c_str(), sent);
        result = true;
      }

      content.close();
    } else {
      Serial.printf("error opening file.\n");
      server.send(500, "text/plain", "500 Internal server error (error opening file)");
      result = true;
    }

    digitalWrite(LED_PIN_ESP, HIGH);

    return result;
  }
      
} staticFileWithLEDHandler;

String getContentType(String filename){
  if(filename.endsWith(".htm"))        return "text/html";
  else if(filename.endsWith(".html"))  return "text/html";
  else if(filename.endsWith(".txt"))   return "text/plain";
  else if(filename.endsWith(".css"))   return "text/css";
  else if(filename.endsWith(".xml"))   return "text/xml";
  else if(filename.endsWith(".png"))   return "image/png";
  else if(filename.endsWith(".gif"))   return "image/gif";
  else if(filename.endsWith(".jpg"))   return "image/jpeg";
  else if(filename.endsWith(".ico"))   return "image/x-icon";
  else if(filename.endsWith(".svg"))   return "image/svg+xml";
  else if(filename.endsWith(".mp3"))   return "audio/mpeg";
  else if(filename.endsWith(".woff"))  return "application/font-woff";
  else if(filename.endsWith(".woff2")) return "application/font-woff2";
  else if(filename.endsWith(".ttf"))   return "application/x-font-ttf";
  else if(filename.endsWith(".eot"))   return "application/vnd.ms-fontobject";
  else if(filename.endsWith(".js"))    return "application/javascript";
  else if(filename.endsWith(".pdf"))   return "application/x-pdf";
  else if(filename.endsWith(".zip"))   return "application/x-zip";
  else if(filename.endsWith(".gz"))    return "application/x-gzip";
  else return "application/octet-stream";
}

void logRequest() {
  Serial.printf("%s %s ", methodToString(server.method()).c_str(), server.uri().c_str());
}

void sendJSON(String body, int statuscode=200) {
  server.send(200, MIMETYPE_JSON, body);
  Serial.printf("%s sent %d bytes.\n", MIMETYPE_JSON.c_str(), body.length());        
}

void handleRoot() {
  logRequest();

  server.sendHeader("Location", "/index.html");
  server.send(302, "text/plain", "302 Found");
  
  Serial.printf("sent redirect.\n");        
}

// Endpoint to request a step to the next number via http GET.
void handleStep() {
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();
  commandNextNumber = true;
  sendJSON("{}");
  
  digitalWrite(LED_PIN_ESP, HIGH);
}

// Endpoint to read the current number via http GET.
void handleCurrentGET() {
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest(); 
  sendJSON("{ \"currentnumber\": " + String(currentNumber) + " }");
        
  digitalWrite(LED_PIN_ESP, HIGH);
}

// Endpoint to list available wifi networks. Does a network scan and then
// serves the result as JSON.
void handleScanGET() {
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();
  
  // Scan for networks and append each one found to the JSON response.
  Serial.printf("Scanning networks...\n");
  int n = WiFi.scanNetworks();
  Serial.printf("Network scan finished, %d networks found.\n", n);
  String body = "[ ";
  for (int i = 0; i < n; i++) { 
    Serial.printf("  %d: %s (%s)\n", i, WiFi.SSID(i).c_str(), ((WiFi.encryptionType(i) == ENC_TYPE_NONE)? "open" : "encrypted"));
    
    if (i>0) { body += ", "; };
    body += "{ \"ssid\": \"" + WiFi.SSID(i) + "\", \"encrypted\": " + ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "false" : "true") + " }";
  }
  body += " ]";

  sendJSON(body);
        
  digitalWrite(LED_PIN_ESP, HIGH);
}

// Endpoint to read the current setup via http GET.
void handleSetupGET() {
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();
  
  String body = "{ \"currentnumber\": " + String(currentNumber);
  body += ", \"ssid\": \"" + ssid + "\"";
  body += ", \"wifimode\": \"" + WiFiMode + "\"";
  body += " }";

  sendJSON(body);
        
  digitalWrite(LED_PIN_ESP, HIGH);
}

// Endpoint to write the current setup. Takes JSON via a http POST.
void handleSetupPOST() {
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();

  // Parse incoming text as JSON.
  StaticJsonBuffer<200> buf;
  JsonObject &json = buf.parseObject(server.arg("plain"));

  // If successful it was at least a syntactically correct
  // JSON object. If not report an error to the client.
  if (!json.success()) {
    String body = "{ \"error\": \"Could not parse JSON request\"}";
    sendJSON(body, 400 /* Bad Request */);
  } else {

    /*
    Serial.printf("\n");
    json.printTo(Serial);
    Serial.printf("\n");
    */

    // Flag to signal that we have seen ssid, passphrase or both in
    // the request. If we did, we need to update the credentials in
    // persistent storage.
    bool store = false;

    if (json.containsKey("ssid")) {
      ssid = json.get<String>("ssid");
      Serial.printf("Found ssid \"%s\".\n", ssid.c_str());
      store = true;
    }

    if (json.containsKey("passphrase")) {
      passphrase = json.get<String>("passphrase");
      Serial.printf("Found passphrase \"%s\".\n", passphrase.c_str());
      store = true;
    }

    if (store) {
      writeSetupToSPIFFS();
    }

    if (json.containsKey("currentnumber")) {
      currentNumber = json.get<int>("currentnumber");
      Serial.printf("Found currentnumber %d for syncing.\n", currentNumber);
      writeNumberToSPIFFS(currentNumber);
      broadcastNumber(currentNumber);
    }
     
    String body = "{}";
    sendJSON(body);
          
    digitalWrite(LED_PIN_ESP, HIGH);
  
    // Restart to let the new settings take effect.
    delay(500);
    Serial.printf("Rebooting...\n");
    ESP.restart();
  }
}

bool looksLikeIP(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

void handleNotFound(){
  digitalWrite(LED_PIN_ESP, LOW);

  logRequest();
  Serial.println("- Not found.");

  // If running as captive portal redirect anything not 
  // found to the setup page.
  if (WiFiMode == "WIFI_AP") {
    if (!looksLikeIP(server.hostHeader()) && server.hostHeader() != (hostName +".local")) {
      Serial.printf("Request redirected to captive portal setup page.\n");
      server.sendHeader("Location", "http://" + server.client().localIP().toString());
      server.send(302, "text/plain", "302 Found");
    } else {
      server.sendHeader("Location", "/setup.html");
      server.send(302, "text/plain", "302 Found");
    }
  } else {
    server.send(404, "text/plain; charset=utf-8", "404 Not Found");
  }
  
  digitalWrite(LED_PIN_ESP, HIGH);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  digitalWrite(LED_PIN_ESP, LOW);

  switch(type) {
    case WStype_DISCONNECTED: {
      Serial.printf("Websocket: [%u] Disconnected!\n", num);
      break;
    }
    case WStype_CONNECTED: {
      Serial.printf("Websocket: [%u] Connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      break;
    }
    case WStype_TEXT: {
      Serial.printf("Websocket: [%u] get Text: %s\n", num, payload);
      String pl = (char*)payload;
      if (pl == "step") {
        commandNextNumber = true;
      }
      break;
    }
  }

  digitalWrite(LED_PIN_ESP, HIGH);
}

void broadcastNumber(int number) {
  // Broadcast new number to all connected websocket-clients.
  String message = String(number);
  webSocket.broadcastTXT(message);
}
  
void loopNetwork() {
  webSocket.loop();
  server.handleClient();
  dnsServer.processNextRequest();
  MDNS.update();
}

void setupNetwork(bool forceAP) {
  // Setup Wifi-LED-pin.
  pinMode(LED_PIN_ESP, OUTPUT);

  hostName = "JarOfPrimes_" + String(ESP.getChipId(), HEX);

  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.hostname(hostName);
  delay(100);

  if (forceAP || ((ssid.length()==0) && (passphrase.length()==0))) {
    setupAsAP();
  } else {
    setupAsSTA();
  }

  Serial.printf("Starting MDNS responder...\n");
  if (MDNS.begin(hostName.c_str(), WiFi.localIP())) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }
  
  Serial.printf("Starting HTTP server...\n");
  server.on("/", handleRoot);
  server.on("/step", handleStep);
  server.on("/current", HTTP_GET, handleCurrentGET);
  server.on("/scan", HTTP_GET, handleScanGET);
  server.on("/setup", HTTP_GET, handleSetupGET);
  server.on("/setup", HTTP_POST, handleSetupPOST);
  server.addHandler(&staticFileWithLEDHandler);
  server.onNotFound(handleNotFound);

  const char * headerkeys[] = {"ETag", "If-None-Match"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );
  
  server.begin();
  
  Serial.printf("Starting websocket server...\n");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.printf("WIFI diagnostics:\n");
  WiFi.printDiag(Serial);
  Serial.printf("\n");
}

