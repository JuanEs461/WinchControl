#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <P1AM.h> // Include P1AM library

// Ethernet configuration (matches your existing setup)
byte mac[] = {0x12, 0x56, 0x78, 0x9A, 0xBC, 0xEF};
IPAddress ip(10, 0, 0, 200);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);  // HTTP server on port 80
const int chipSelect = 28;  // SD card CS pin on P1AM (pin 28)

void setup() {
  Serial.begin(115200);
  
  // Initialize P1AM modules
  if (!P1.init()) {
    Serial.println("P1AM initialization failed!");
    while (1); // Halt on failure
  }
  
  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized");
  
  // Initialize Ethernet
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("P1AM Web Server IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("Place 'control.htm' on SD card for web interface");
}

void loop() {
  EthernetClient client = server.available();
  
  if (client) {
    Serial.println("New client connected");
    String request = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        // End of HTTP request
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          // Handle button press
          if (request.indexOf("GET /button") != -1) {
            Serial.println(request);
          }
          
          // Serve HTML file from SD card
          File webFile = SD.open("HTMLTest.htm");
          
          if (webFile) {
            sendHttpResponse(client, 200, "text/html");
            // Send file contents to client
            while (webFile.available()) {
              client.write(webFile.read());
            }
            webFile.close();
          } else {
            sendHttpResponse(client, 404, "text/plain");
            client.println("404: File Not Found - Place control.htm on SD card");
          }
          break;
        }
      }
    }
    
    delay(1);  // Allow client to process
    client.stop();
    Serial.println("Client disconnected");
  }
}

// Helper function to send HTTP response headers
void sendHttpResponse(EthernetClient &client, int code, const char* contentType) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.println(" OK");
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  client.println(); // End of headers
}
