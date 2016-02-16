#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>


//=======Declaring functions==============
void callback(char* topic, byte* payload, unsigned int length);
void colorWipe(uint32_t c, uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
uint32_t Wheel(byte WheelPos);


//=================NeoPixels=====================

#define PIXEL_PIN    12    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 60     // Number of neopixels in strip

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

int receivedRGBT[4] = {0,0,0,0}; //used to store the RGB and time (s) values sent to the bot over MQTT, after they're converted to int

//==================WiFi=====================

char wifissid[] = "XXXXXXXXXXXX";         // Wifi network name or ESSID/SSID
char wifipwd[] = "XXXXXXXXXXXXXX";   // Wifi password

WiFiClient wifiClient;


//====================MQTT====================
#undef MQTT_KEEPALIVE     // redefine mqtt keepalive period in seconds, about 3 loops
#define MQTT_KEEPALIVE 60
char nodeprefix[] = "neoPixelBot";   // Prefix for node name rest is mac address
char mqttsrvr[] = "10.3.11.253";    // Ip address of mqtt broker
//char mqttsrvr[] = "192.168.1.82";    // Ip address of mqtt broker
char mqttuid[] = "";   // mqtt broker username
char mqttpwd[] = ""; // mqtt broker password

char topic_node[]   = "SHHNoT/commons/node/neoPixelBot";   // topic name for LWT node up/down notifications
char topic_switch[] = "SHHNoT/commons/actuator/neoPixelBot/switch";   // topic name for switch
String NodeName="";

PubSubClient client(mqttsrvr, 1883, callback, wifiClient);


// Convert 6 byte mac address to text string
String macToStr(const uint8_t* mac)
{
   String result;
   
   for (int i = 0; i < 6; ++i) {
      result += String(mac[i], 16);
   }
   
   return result;
}

// Connect to Wifi
void WifiConnect() {
  int timout = 30;
  
  Serial.print("Wifi connecting to ");
  Serial.println(wifissid);
  WiFi.begin(wifissid, wifipwd);
  while (WiFi.status() != WL_CONNECTED) {
     delay(1000);
     Serial.print(".");
     if(--timout < 0){
        Serial.println("WIFI connect failed reset and try again...");
        //abort();
     }       
  }
  Serial.println("");
  Serial.print("WiFi connected with IP ");
  Serial.println(WiFi.localIP()); 
  return;
}

// Connect to MQTT Broker
void BrokerConnect() {
   uint8_t mac[6];
   String lwtUp;
   String lwtDn; 
  
   if(NodeName.length() <= 0){
      // Generate client name and LWT messages based on MAC address and last 8 bits of microsecond counter
      WiFi.macAddress(mac);   
      NodeName = nodeprefix + macToStr(mac);
      lwtUp = NodeName + " is alive";
      lwtDn = NodeName + " is dead";
   } 
  
   Serial.print(NodeName);
   Serial.print(" connecting to ");
   Serial.print(mqttsrvr);
   Serial.print(" as ");
   Serial.println(mqttuid);

   if (client.connect((char*)NodeName.c_str(), mqttuid, mqttpwd, topic_node, 0, 0,  (char*)lwtDn.c_str())) {
      Serial.println("Connected to MQTT broker");
      Serial.print("LWT topic is: ");
      Serial.println(topic_node);
      if (client.publish(topic_node, (char*)lwtUp.c_str())) {
         Serial.println("Publish lwt up ok");
      } else {
         Serial.println("Publish lwt up failed");
      }
      if (client.subscribe(topic_switch)){
        Serial.println("Subscribed!");
      } else {
        Serial.println("Subscription failed");
      }
   } else {
      Serial.println("MQTT connect failed, reset and try again...");
      //abort();
   }
  
   return; 
}  

// arrived MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
   Serial.println("hello!");
   String  assembleString = "";   //Used to store the incoming MQTT message as it's read from the stream
   int pos = 0;                   //keep track of where in the incoming MQTT message we are. 0 == Red value, 1 == Green, 2 == Blue, 3 == time (s).
   for (int i=0; i<length; i++){
     //Serial.println((char)payload[i]);
     if (isDigit((char)payload[i])){
       assembleString += (char)payload[i];
     }
       
     if (!isDigit((char)payload[i])){
       //Serial.println(assembleString);
       int num = assembleString.toInt();
       receivedRGBT[pos] = constrain(num,0,255);
       pos++;
       assembleString="";
       
       Serial.print("After toint: ");
       Serial.println(num);
     }
     
     if (i == length-1){
       if (isDigit((char)payload[i])){ // If the last character of the payload is a number, it should be the final digit of the "seconds" value. So put it in the RGBT array...
         int num = assembleString.toInt();
         assembleString = "";
         receivedRGBT[3] = constrain(num,0,60);
         if (pos < 3){                //...unless we haven't reached the third seperator in the message yet, in which case the seconds value is missing from the message and will default to 15.
           receivedRGBT[3] = 15;
         }
       }
     }
   }
     
      
   Serial.print("ReceivedRGBT == ");
   for (int i=0; i<4; i++){
     Serial.print(receivedRGBT[i]);
     Serial.print(" ");
   }
   Serial.println("");
   
   colorWipe(strip.Color(receivedRGBT[0],receivedRGBT[1], receivedRGBT[2]), 50);
   theaterChase(strip.Color(receivedRGBT[0], receivedRGBT[1], receivedRGBT[2]), 50);
 
   return;
} 



void setup() {

  strip.begin();// Instantiate the Neopixel strip
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(115200);
   
  Serial.println("\n\nInitializing node");
  
  WifiConnect();
 
  BrokerConnect();   
   
}

void loop() {
  
  
   // Reconnect to wifi if connection is lost
   if(WiFi.status() != WL_CONNECTED){
      WifiConnect();
   }
  
   // Reconnect to broker if conenction is lost
   if(!client.connected()) {
      BrokerConnect();
   }   
   rainbowCycle(20);
   client.loop(); //Keep the MQTT client ticking over in the background, listening for messages
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}


void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
    client.loop();
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  //for (int j=0; j<10; j++) {  //do 10 cycles of chasing
  int timeStarted = millis();
  while(millis() - timeStarted < receivedRGBT[3] * 1000){  //Loop this pattern until we exceed the number of seconds stored in receivedRGBT
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
