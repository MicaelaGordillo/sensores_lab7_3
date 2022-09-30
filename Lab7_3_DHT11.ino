#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include "config.h"
#define PIN_LM35 32
#define factor 0.0805860805860
int datoVal;
float milliVolt,tempC,tempF,humedad;
#define ADC_VREF_mV    3300.0 // 3.3v en millivoltios
#define ADC_RESOLUTION 4096.0

// Creamos el servidor AsyncWebServer en el puerto 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 1000; // actualizacion cada segundo//30000;

// Tomando datos del sensor para mandarlos a objeto JSON
String getSensorReadings(){

  readings["temperatureC"] = String(tempC);
  readings["temperatureF"] = String(tempF);
  readings["humidity"] =  String(humedad);
  
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Inicializando LittleFS
void initFS() {
 // Iniciamos  SPIFFS
  if(!SPIFFS.begin())
     { Serial.println("ha ocurrido un error al montar SPIFFS");
       return; }
}

// Inicializando WiFi
void initWiFi() {
// conectamos al Wi-Fi
  WiFi.begin(ssid, password);
  // Mientras no se conecte, mantenemos un bucle con reintentos sucesivos
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      // Esperamos un segundo
      Serial.println("Conectando a la red WiFi..");
    }
  Serial.println();
  Serial.println(WiFi.SSID());
  Serial.print("Direccion IP:\t");
  // Imprimimos la ip que le ha dado nuestro router
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  initWiFi();
  initFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");
  
  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  // Lectura de los datos del sensor
  datoVal = analogRead(PIN_LM35);
  // Convirtiendo los datos del ADC a milivoltios
  milliVolt = datoVal * (ADC_VREF_mV / ADC_RESOLUTION);
  // Convirtiendo el voltaje al temperatura en °C
  tempC = datoVal * factor ;
  // convirtiendo °C a °F
  tempF = tempC * 9 / 5 + 32;

  // Humedad
  humedad = datoVal/10;
  
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 30 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
  
  // Imprimiendo valores en el monitor serial:
  Serial.print("Lectura del ADC: ");
  Serial.print(datoVal);   // Valor leido por el ADC
  Serial.print("  Temperatura: ");
  Serial.print(tempC);   // Imprimiendo la temperatura en °C
  Serial.print("°C");
  Serial.print("  ~  "); //  
  Serial.print(tempF);   // Imprimiendo la temperatura en °F
  Serial.print("°F");
  Serial.print(" Humedad: ");
  Serial.println(humedad);
  delay(500);
}
