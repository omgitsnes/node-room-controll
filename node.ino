#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// Libraries used for DS18B20 sensor (Temperature sensor)
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "PowerPuffGurls";
const char* password = "f30b060e418a";

// How many bits to use for temperature values: 9, 10, 11 or 12
#define SENSOR_RESOLUTION 9
// Index of sensors connected to analogPin, default: 0
#define SENSOR_INDEX 0

// Pins
int ledPin = LED_BUILTIN; // GPIO13
int buzzerPin = D8;
// Analog
int analogPin = A0; 

// Used for DS18B20 sensor
OneWire oneWire(analogPin);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;


WiFiServer server(80);

void setup() {
    Serial.begin(115200);
    delay(10);

    pinMode(buzzerPin, OUTPUT);
    pinMode(ledPin, OUTPUT);

    //Make the led "off" by default
    digitalWrite(ledPin, HIGH);

    //Connect to WiFi network
    Serial.println();
    Serial.println("------");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    Serial.println("------");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");

    //Start the server
    server.begin();
    Serial.println("Server started");

    //Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

    // More DS18B20 stuff
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);

}

void loop() {
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    // Wait until the client sends some data, if no data is recieved for 5 seconds reset the loop so it does not get stuck
    int counter = 0;
    Serial.println("new client");
    while(!client.available()){
        delay(1);
        counter++;
        if (counter > 5000)
        {
            return;
        }

    }


    String request = handleRequest(client);
    // Match the request
    int value = handleLED(request);
    float lux = readLDR();
    //float temperatureInCelsius = readTemperature(); //TODO
    int buzz = handleBuzzer(request);


    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<meta http-equiv=\"refresh\" content=\"1000\" >");
    client.println("<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\" integrity=\"sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm\" crossorigin=\"anonymous\">");
    client.println("    <script defer src=\"https://use.fontawesome.com/releases/v5.0.9/js/all.js\" integrity=\"sha384-8iPTk2s/jMVj81dnzb/iFR2sdA7u06vHJyyLlAd4snFpCl/SnyUjRrbdJsw1pGIl\" crossorigin=\"anonymous\"></script>");
    client.println("</head>");
    client.println("<body>");

    client.println("<div class=\"container-fluid row\">");
    client.print("<div class=\"col-md-4 bg-light border text-center\">");
    client.println("<br><h2 class=\"\"><i class=\"far fa-lightbulb\"></i> Iluminacao</h2><hr><p>");
    if (lux <= 1.0) {
        client.print("<h4 class=\"text-danger\">Ligado</h4>");
    }
    if (lux > 1.0 && lux < 2.0) {
        client.print("<h4 class=\"text-warning\">Alguma luz</h4>");
    }
    if (lux >= 2.0) {
        client.print("<h4 class=\"text-success\"><i class=\"fas fa-moon\"></i>Desligado</h4>");
    }

    client.print(" (");
    client.print(lux);
    client.print(") ");
    client.println("</p><br><br></div>");
    client.println("<div class=\"col-md-4 bg-light border text-center\">");
    client.println("<br><h2 class=\"\"><i class=\"fas fa-wrench\"></i> Controlos de Iluminacao</h2><hr><p>");

    client.println("<a href=\"/LED=OFF\"\"><button class=\"btn btn-danger\">Turn OFF </button></a>");
    client.println("<a href=\"/LED=ON\"\"><button class=\"btn btn-success\">Turn On </button></a><br />");  
    client.println("<br><br></div>");

    client.println("<div class=\"col-md-4 bg-light border text-center\">");
    client.println("<br><h2 class=\"\"><i class=\"far fa-clock\"></i> Alarmes</h2><hr><p>");

    if (buzz == 0) {
        client.print("(NO BUZZ)<br>");
    }
    if (buzz == 1) {
        client.print("(BUZZ)<br>");
    }
    if (buzz == 2) {
        client.print("(SUPERBUZZ)<br>");
    }


    client.println("<a href=\"/SUPERBUZZ\"\"><button class=\"btn btn-danger \">SUPERBUZZ </button></a>");
    client.println("<a href=\"/BUZZ\"\"><button class=\"btn btn-warning \">BUZZ </button></a>");
    client.println("<a href=\"/\"\"><button class=\"btn btn-info \"> NO BUZZ </button></a>");
    client.println("</p></div>");


    client.println("</div></body></html>");

    delay(1);
    Serial.println("Client disconnected");
    Serial.println("");
}

String handleRequest(WiFiClient client) {
    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
    return request;
}

float readLDR() {
    // Handle switch

    float lux = analogRead(analogPin);
    return lux; 
}

float readTemperature() {
    // Handle switch

    sensors.requestTemperatures();
    // Measurement may take up to 750ms

    float temperatureInCelsius = sensors.getTempCByIndex(SENSOR_INDEX);
    return temperatureInCelsius; 
}
/*
 * Ativates the buzzer if it need to
 * Returns new buzzer state
 *
 */
int handleBuzzer(String request) {
    int buzz = 0;
    if (request.indexOf("/BUZZ") != -1)  {
        tone(buzzerPin, 1000);
        delay(1000);
        noTone(buzzerPin);
        buzz = 1;
    }

    if (request.indexOf("/SUPERBUZZ") != -1)  {
        for (int i = 0; i < 20; ++i)
        {
            tone(buzzerPin, 5000);
            delay(100);
        }

        noTone(buzzerPin);
        buzz = 2;
    }
    return buzz;
}

/*
 * Activates the LED f it needs to
 * Return new LED state
 */

int handleLED(String request) {
    int value = LOW;
    if (request.indexOf("/LED=OFF") != -1)  {
        digitalWrite(ledPin, HIGH);
        value = HIGH;
    }
    if (request.indexOf("/LED=ON") != -1)  {
        digitalWrite(ledPin, LOW);
        value = LOW;
    }
    return value;
}