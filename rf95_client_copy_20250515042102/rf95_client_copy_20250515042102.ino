

// ROBIOT
// For Arduino UNO/Nano!

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 4
#define RFM95_RST 2
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 863.0

struct packet{
  uint8_t droneID;
  int32_t latitude;
  uint8_t NorthSouth;
  int32_t longitude;
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

  // Singleton instance of the radio driver
  RH_RF95 rf95(RFM95_CS, RFM95_INT);

  uint8_t latOrLong = 0;
  struct packet txPacket;
  uint8_t SEQ = 0;

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  //while (!Serial);
  Serial.begin(74880);
  delay(100);

  Serial.println("Arduino LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    delay(5);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);
}

float latitudeInput = 43.3559;
float longitudeInput = -3.1222;
int16_t packetnum = 0;  // packet counter, we increment per xmission

uint8_t buf[sizeof(struct packet)];

void loop()
{
  txPacket.droneID = 1;
  txPacket.latitude = latitudeInput * 10000;
  txPacket.NorthSouth = 0;
  txPacket.longitude = longitudeInput * 10000;
  txPacket.EastWest = 0;
  txPacket.altitude = 5;
  txPacket.speed = 20;
  txPacket.SEQ = SEQ;
  SEQ ++;

  uint8_t* radiopacket[sizeof(txPacket)];
  memcpy(radiopacket, &txPacket, sizeof(txPacket));

  Serial.println("=======================");
  Serial.println(sizeof(float));

  Serial.println((char*)radiopacket);
  Serial.println(sizeof(radiopacket));

  Serial.println(txPacket.droneID);
  Serial.println(txPacket.latitude);
  Serial.println(txPacket.longitude);

  Serial.println("=======================");

  //Serial.println("Sending..."); delay(10);
  memcpy(&buf, &txPacket, sizeof(txPacket));

  rf95.send((uint8_t *)buf, sizeof(buf));
  Serial.println(sizeof(radiopacket));
  Serial.println("Waiting for packet to complete...");delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply

  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  //Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      //Serial.print("Got reply: ");
      //Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  while(!Serial.available()){
    if(latOrLong == 0){
      Serial.println("LATITUDE SIMULATUA SARTU:");
      delay(10000);
      if (Serial.available()) {
        latitudeInput = Serial.parseFloat();
        latOrLong = 1;
      }
      latOrLong = 1;
    }
    else{
      Serial.println("LONGITUDE SIMULATUA SARTU:");
      delay(10000);
      if (Serial.available()) {
        longitudeInput = Serial.parseFloat();
        latOrLong = 0;
      }
      latOrLong = 0;
    }

    //delay(1000);
  }

  while (Serial.available()) {
    Serial.read();
  }

}