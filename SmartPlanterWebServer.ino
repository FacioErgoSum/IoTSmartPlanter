/*********
  Written by Geoff McIntyre
  View complete project details at: 

  Based on code by Rui Santos
  https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-web-server-arduino-ide/

  Thank you for checking out my project!
*********/

// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

// Replace with your network credentials
const char* ssid = "Network SSID";
const char* password = "Network Password";

#define relay_pin 15
#define soil_moist 36
#define water_level 39

//Variables for Buzzer 
int freq = 261.63; //Set frequency of the buzzer, this is a C note
int channel = 0;
int resolution = 8;

//Boolean for light status
bool light = true;

//Set thresholds for sensors (Tweak depending on plant/soil type)
float highWater = 1000;
float lowWater = 700;

float highMoist = 1200;
float lowMoist = 500;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//Returns soil moisture reading
String readSoilMoisture() {
  float m = analogRead(soil_moist);
  if (isnan(m)) {    
    Serial.println("Failed to read from Soil Moisture sensor!");
    return "--";
  }
  else {
    Serial.println(m);
    //Sets text depending on values
    if(m > highMoist){
      return String("High");
    } else if (m > lowMoist) {
      return String("Low");
    } else {
      //Beeps the buzzer for 250ms
      ledcWriteTone(channel, freq);
      delay(250);
      ledcWriteTone(channel, 0);
      Serial.println("Buzzer Activated");
      return String("Dry");
    }
  }
}

//Returns water level reading
String readWaterLevel() {
  float l = analogRead(water_level);
  if (isnan(l)) {
    Serial.println("Failed to read from Water Level sensor!");
    return "--";
  }
  else {
    Serial.println(l);
    //Sets text depending on values
    if(l > highWater){
      return String("High");
    } else if (l > lowWater) {
      return String("Low");
    } else {
      //Beeps the buzzer for 250ms
      ledcWriteTone(channel, freq);
      delay(250);
      ledcWriteTone(channel, 0);
      Serial.println("Buzzer Activated");
      return String("Empty");
    }
  }
}

//Toggles the light status
String toggleLight() {
  if(light){
    light = false;
    //Activates relay
    digitalWrite(relay_pin, LOW);
    return String("OFF");
  } else if (!light){
    light = true; 
    //Deactivates relay
    digitalWrite(relay_pin, HIGH);
    return String("ON");
  }
}

//The HTML code
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    button { 
      background-color: #444; /* Dark Grey */
      border: none;
      color: white;
      padding: 15px 60px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      border-radius: 10px;
      font-size: 16px; 
    }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 Smart Planter</h2>
  <p>
    <i class="fas fa-tint" style="color:#009bc0;"></i> 
    <span class="dht-labels">Soil Moisture:</span> 
    <span id="soil-moisture">%SOILMOISTURE%</span>
  </p>
  <p>
    <i class="fas fa-water" style="color:#00add6;"></i> 
    <span class="dht-labels">Water Level:</span>
    <span id="water-level">%WATERLEVEL%</span>
  </p>
  <p>
    <a>
      <h3>Toggle Light:</h3>
      <button onclick="toggleLightJS()">
        <i class="far fa-lightbulb fa-3x" style="color:#FFF;"></i>
      </button>
    </a>
  </p>
</body>
<script>

//Function that toggles light. Called on button press
function toggleLightJS () {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "/toggle-light", true);
  xhttp.send();
}

//Function that refreshes soil moisture value every 1000ms
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("soil-moisture").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/soil-moisture", true);
  xhttp.send();
}, 10000 ) ;

//Function that refreshes water level value every 1000ms
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("water-level").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/water-level", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

//Updates variable text with current values
String processor(const String& var){
  //Serial.println(var);
  if(var == "SOILMOISTURE"){
    return readSoilMoisture();
  }
  else if(var == "WATERLEVEL"){
    return readWaterLevel();
  }
  return String();
}

//Runs at the start
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  //Setup the buzzer
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(25, channel);

  //Setup the pinMode for the relay
  pinMode (relay_pin, OUTPUT); 
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/soil-moisture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSoilMoisture().c_str());
  });
  server.on("/water-level", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readWaterLevel().c_str());
  });
  server.on("/toggle-light", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", toggleLight().c_str());
  });
  
  // Start server
  server.begin();
}
 
void loop(){
  //Nothing goes here because we are running an Asynchronous Web Server.
}
