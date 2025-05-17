#include <WiFi.h>

// AP mode configuration - Changed SSID and password
const char* ap_ssid = "ESP32-AP";
const char* ap_password = "12345678";

// Variables from the original code
String header;
String output26State = "off";
String output27State = "off";
const int output26 = 26;
const int output27 = 27;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000); // Delay for stabilization
  
  // Test LEDs at startup
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  
  digitalWrite(output26, LOW);
  digitalWrite(output27, HIGH);
  Serial.println("LEDs turned ON for testing");
  delay(2000);
  
  digitalWrite(output26, HIGH);
  digitalWrite(output27, LOW);
  Serial.println("LEDs turned OFF");
  
  // Configure WiFi in AP mode
  Serial.print("Setting up Access Point: ");
  Serial.println(ap_ssid);
  
  // Disconnect from any previous WiFi
  WiFi.disconnect(true);
  delay(1000);
  
  // Set mode to AP
  WiFi.mode(WIFI_AP);
  
  // Configure static IP
  IPAddress local_IP(192,168,50,1);
  IPAddress gateway(192,168,50,1);
  IPAddress subnet(255,255,255,0);
  
  if(!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("AP Config Failed");
  } else {
    Serial.println("AP Config Success");
  }
  
  // Start AP with different channel
  bool apStarted = WiFi.softAP(ap_ssid, ap_password, 1); // Use channel 1
  
  if(apStarted) {
    Serial.println("AP setup success!");
  } else {
    Serial.println("AP setup failed!");
  }
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
  Serial.println("Server started!");
  Serial.print("Connect to WiFi network: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("Then access the web server at: http://");
  Serial.println(IP);
}

void loop() {
  // Print status periodically to confirm server is running
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    lastStatusTime = millis();
    Serial.print("AP running. Connected stations: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
  
  WiFiClient client = server.available();   // Check for connected clients

  if (client) {                             // If a new client is connected
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client connected!");
    String currentLine = "";
    
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code and content-type
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Control GPIOs based on URL requests
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server (AP Mode)</h1>");
            
            // Display current state and ON/OFF buttons for GPIO 26
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state and ON/OFF buttons for GPIO 27
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            
            client.println("</body></html>");
            client.println();
            break;
            
          } else { // If you got a newline, clear currentLine
            currentLine = "";
          }
          
        } else if (c != '\r') { // If you got anything else but a carriage return character
          currentLine += c;     // Add it to the end of the currentLine
        }
      }
    }
    
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
