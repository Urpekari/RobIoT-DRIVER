#define BYTESWAP32(z) ((uint32_t)((z&0xFF)<<24|((z>>8)&0xFF)<<16|((z>>16)&0xFF)<<8|((z>>24)&0xFF)))

// ROBIOT
// FOR ESP8266!

#include <SPI.h>
#include <RH_RF95.h>
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define RFM95_CS 5
#define RFM95_RST 16
#define RFM95_INT 4

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 863.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

struct packet{
  uint8_t droneID;
  uint8_t lat0;
  uint8_t lat1;
  uint8_t lat2;
  uint8_t lat3;
  uint8_t NorthSouth;
  uint8_t lon0;
  uint8_t lon1;
  uint8_t lon2;
  uint8_t lon3;
  uint8_t EastWest;
  uint16_t altitude;
  uint16_t speed;
  
  // Keeping track of the packets
  float time;
  uint8_t SEQ;

  // TBD how we send data
  uint8_t sensorid;
  uint32_t sensorData;

  //This should always be 0!
  uint8_t end;
};

  struct packet txPacket;
  uint8_t SEQ = 0;

int sendOHttp(uint8_t robiotID, float gpsLat, uint8_t noSo, float gpsLon, uint8_t eaWe, float gpsAlt);

#define SERVER_IP "192.168.1.35:5000"

//TODO: Nire etxeko wifi pasahitza ez publikatzea hortik... ya berandu... en fin... :(
  
#ifndef STASSID
#define STASSID "Portuetxe24"
#define STAPSK "ErEkIk27Ct"
#endif

int robiotID = 1;
int gwID = 1;
float gpsLat = 0;
float gpsLon = 0;
float gpsAlt = 0;
float hdg = 0;

// Blinky on receipt
#define LED 13

void setup() 
{
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  while (!Serial);
  Serial.begin(74880);
  delay(100);

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    delay(5);
    ESP.wdtFeed();
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    ESP.wdtFeed();
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void loop()
{
  if (rf95.available())
  {

  //Replacing the arbitrary 255-4 buffer lenght with what we need
  uint8_t buf[sizeof(struct packet)];
  uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      //Serial.println((char*)buf);
      
      struct packet rxPacket;
     
      Serial.println("=======================");
      Serial.println(sizeof(float));

      Serial.println((char*)buf);
      Serial.println(sizeof(buf));
      gpsLat = 16777216*rxPacket.lat3 + 65536*rxPacket.lat2 + 256*rxPacket.lat1 + rxPacket.lat0;
      int32_t longitude = 16777216*rxPacket.lon3 + 65536*rxPacket.lon2 + 256*rxPacket.lon1 + rxPacket.lon0;

      memcpy(&rxPacket, &buf, sizeof(rxPacket));
      Serial.println(rxPacket.droneID);
      Serial.println(16777216*rxPacket.lat3 + 65536*rxPacket.lat2 + 256*rxPacket.lat1 + rxPacket.lat0);
      Serial.println(16777216*rxPacket.lon3 + 65536*rxPacket.lon2 + 256*rxPacket.lon1 + rxPacket.lon0);

      //LoRa bidez jasotakoa http bidez bidali
      sendOHttp(rxPacket.droneID, (float)((16777216*rxPacket.lat3 + 65536*rxPacket.lat2 + 256*rxPacket.lat1 + rxPacket.lat0)/(float)10000), rxPacket.NorthSouth, (float)((16777216*rxPacket.lon3 + 65536*rxPacket.lon2 + 256*rxPacket.lon1 + rxPacket.lon0)/(float)10000), rxPacket.EastWest, rxPacket.altitude);

      Serial.println("=======================");
      Serial.println(sizeof(rxPacket));
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      
      // Send a reply
      uint8_t data[] = "Ack";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

int sendOHttp(uint8_t robiotID, float gpsLat, uint8_t noSo, float gpsLon, uint8_t eaWe, float gpsAlt) {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

    if(noSo > 0){
      gpsLat = gpsLat- 2*gpsLat;
    }


    if(eaWe > 0){
      gpsLon = gpsLon- 2*gpsLon;
    }

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    
    http.begin(client, "http://" SERVER_IP "/gwInsert/1");  // HTTP
    http.addHeader("Content-Type", "application/json");

    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    char buffer[1024];
      sprintf(buffer, "{\"robiotId\":\"%d\",\n\"gwid\":\"%d\",\n\"lat\":\"%.6f\",\n\"lon\":\"%.6f\",\n\"hdg\":\"%.6f\",\n\"alt\":\"%.6f\"}", robiotID, gwID, gpsLat, gpsLon, hdg, gpsAlt);
    int httpCode = http.POST(buffer);

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        // int startOfLon = payload.lastIndexOf(' ');
        // int endOfLon = payload.lastIndexOf('\n');
        // int endOfLat = payload.lastIndexOf(' ', startOfLon - 1);
        // int startOfLat = payload.lastIndexOf(',');
        // Serial.printf("SLO %d ELO %d SLA %d ELA %d \n", startOfLon, endOfLon, startOfLat, endOfLat);
        // Serial.printf("LATITUDE: %s\n LONGITUDE %c\n", payload.substring(startOfLat, startOfLon), payload[startOfLat]);
        // //Serial.printf("LATITUDE: %s\n LONGITUDE %s\n", payload.substring(startOfLat, endOfLat), payload.substring(startOfLon, endOfLon));
        // //TODO: JASOTAKO HAU PARSEATU ETA DRONEARI HURRENGO WAYPOINTAREN BALIOAREKIN ERANTZUN!
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }  
  return 0;
}
