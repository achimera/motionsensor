/*
 Copyright (C) 2014 Andrey Shigapov <andrey@shigapov.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/*-----( Declare Constants )-----*/
#define RELAY_ON 0
#define RELAY_OFF 1

#define Relay_1  8  // Digital I/O pin number
#define Relay_2  7  // Digital I/O pin number

//struct for wireless data transmission
typedef struct {
  int nodeID;             //node ID (1xx, 2xx, 3xx); 1xx = hall, 2xx = main floor, 3xx = outside
  int deviceID;           //sensor ID (2, 3, 4, 5)
  unsigned long var1_usl; //uptime in msecs
  float var2_float;       //could contain sensor data
  float var3_float;       //could contain more sensor data
} Payload;
Payload sensorData;

//
// Hardware configuration
//
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9,10);

int ledRX = 13; // Arduino Digital I/O pin number
int ledTX = 13;
int ledOn = 5;

//
// Topology
//

// Radio pipe addresses 
const uint64_t pipes[2] = { 0xE7E7E7E700LL, 0xE7E7E7E700LL };

char dataToSend = 'A';
bool bRelayOn = false;

void relayOn()
{
    digitalWrite(ledOn, HIGH);
    if(!bRelayOn) {
      digitalWrite(Relay_1, RELAY_ON);
      delay(300);
    }
    bRelayOn = true;
}

void relayOff()
{
    digitalWrite(Relay_1, RELAY_OFF);
    digitalWrite(ledOn, LOW);
    bRelayOn = false;
}

void relayToggle()
{
    if(bRelayOn){
        relayOff();
    } else {
        relayOn();
    }
}


void setReceivingRole()
{
    uint64_t txPipe = pipes[1];
    uint64_t rxPipe = pipes[0];
  
    printf("\r\n*** Receiving mode ***\r\n");
  
    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(1, rxPipe);
   
    printf("Listening on %#1x%lx\r\n", ((char*)&rxPipe)[4] & 0x000ff, rxPipe);
    printf("Writing to  %#1x%lx\r\n", ((char*)&txPipe)[4] & 0x000ff, txPipe);
    printf("\r\n");
}

void setup(void)
{
  relayOff(); //keep the relay turned off when initializing
  
  pinMode(ledRX, OUTPUT);     
  pinMode(ledTX, OUTPUT);     
  pinMode(ledOn, OUTPUT);     
  pinMode(Relay_1, OUTPUT);     
  
  digitalWrite(ledRX, HIGH);
  digitalWrite(ledTX, HIGH);
  digitalWrite(ledOn, HIGH);
  delay(1000);               // wait for a second
  digitalWrite(ledRX, LOW);
  digitalWrite(ledTX, LOW);
  digitalWrite(ledOn, LOW);

  //
  // Print preamble
  //
  Serial.begin(57600);
  printf_begin();
  printf("\r\n Sensor Wireless Receiver v0.4\n\r");

  //
  // Setup and configure rf radio
  //
  radio.begin();
  
  // set output level
  radio.setPALevel(RF24_PA_LOW);

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,5);
  radio.setAutoAck(true);
  
  // set CRC to 8 bit
  radio.setCRCLength(RF24_CRC_8);
  
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(15);

  // reduce the payload size.  seems to
  // improve reliability
  //radio.setPayloadSize(1);
  radio.setPayloadSize(sizeof(sensorData));

  //
  // Open pipes to other nodes for communication
  //
  setReceivingRole();

  //
  // Start listening
  //
  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails();

  printf("Receiver initialization completed.\r\n");
  
}



///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  delay(50);     

  // if there is data ready
  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    char data = 0;
    bool done = false;
    bool reply = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      //done = radio.read( &data, sizeof(data) );
      done = radio.read( &sensorData, sizeof(sensorData) );
      // Spew it
#ifdef ENABLE_SERIAL
      printf("\r\nReceived Sensor data from nodeID '%u'.", sensorData.nodeID);
      printf("\r\nReceived Sensor data from deviceID '%u'.", sensorData.deviceID);
      printf("\r\nReceived Sensor data timestamp '%lu'.", sensorData.var1_usl);
      //printf("\r\nReceived data var2_float '%f' ...", data.var2_float);
      //printf("\r\nReceived data var3_float '%f' ...", data.var3_float);
#endif //#ifdef ENABLE_SERIAL

      /*
      printf("\r\nReceived data '%c' ...", data);
      
      if(data == 'o') { // turn relay ON
        printf("Switch ON the relay.");
        relayOn();
        delay(1000);
        printf("Switch OFF the relay.");
        relayOff();
        //relayToggle();
        
      } else if(data == 'f') { // turn relay OFF
        relayOff();
        
      } else if((data == 't') ||(data == 'T')) { // toggle relay
        relayToggle();
        reply = (data == 't');
        
      } else if(data == 's') { // report status
        data = bRelayOn ? 'o' : 'f';
      }
      */
    }
    
    // First, stop listening so we can talk
    radio.stopListening();
    digitalWrite(ledRX, HIGH);   // turn the LED on (HIGH is the voltage level)
      // Delay just a little bit to let the other unit
      // make the transition to receiver
    delay(50);
    digitalWrite(ledRX, LOW);   
 
    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }

  //
  // Serial interface
  //
  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'D' ) {
        // print details
        radio.printDetails();
        
    } else if ( c == 'T' ) {
        relayToggle();
        
    } else if ( c == 'O' ) {
        relayOn();
        
    } else if ( c == 'F' ) {
        relayOff();
    }
  }
}
