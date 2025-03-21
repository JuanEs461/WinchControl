#include <SD.h>               // For SD card
#include <Ethernet.h>         // For Ethernet communication
#include <SPI.h>              // For SPI communication

// Ethernet and server settings
byte mac[] = { 0x12, 0x56, 0x78, 0x9A, 0xBC, 0xEF };  // MAC address
IPAddress ip(10, 0, 0, 200); // Static IP
EthernetServer server(80);    // HTTP server on port 80

// SD card chip select pin
const int chipSelect = 28;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial (optional)

  // Initialize Ethernet with DHCP fallback
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Using static IP (DHCP failed).");
    Ethernet.begin(mac, ip); // Assign static IP
  }

  // Check Ethernet hardware
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet hardware not found!");
    while (true); // Halt
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable disconnected!");
  }

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed. Logging disabled.");
  }

  // Start the server
  server.begin();
  Serial.print("Server IP: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // Handle Ethernet client non-blockingly
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Client connected.");
    String currentLine = "";
    bool isPost = false;
    String postData = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (isPost) {
          postData += c;
        }

        // End of HTTP request
        if (c == '\n' && currentLine.length() == 0) {
          if (isPost) {
            handlePostRequest(client, postData);
          } else {
            serveHtml(client);
          }
          break;
        }

        // Identify request type
        if (currentLine.startsWith("POST")) {
          isPost = true;
        }

        // Track current line
        if (c == '\n') {
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }
}

void serveHtml(EthernetClient& client) {
  File htmlFile = SD.open("SCHEDULE.htm");
  if (htmlFile) {
    Serial.println("Serving SCHEDULE.htm");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    while (htmlFile.available()) {
      client.write(htmlFile.read());
    }
    htmlFile.close();
  } else {
    Serial.println("404: SCHEDULE.htm not found");
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("404 File Not Found");
  }
}

void handlePostRequest(EthernetClient& client, String& postData) {
  Serial.println("POST data received:");
  Serial.println(postData);

  // Example: Parse JSON data (use ArduinoJson for better parsing)
  if (postData.indexOf("\"mode\":\"Time\"") > -1) {
    Serial.println("Mode: Time");
  } else if (postData.indexOf("\"mode\":\"Trigger\"") > -1) {
    Serial.println("Mode: Trigger");
  }

  // Respond to the client
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("Task received and processed.");
}