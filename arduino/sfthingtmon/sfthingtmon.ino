// sfthingtmon.ino
// Glen Wiley <glen.wiley@gmail.com>
//
// write diag messages to Serial at 9600bps
// connect to server via Wifi and send readings at intervals
// <MACADDR> <TEMP1> <TEMP2> <TEMP3> <CT>
//
// blinks LED every 100ms when trying to connect
// turns on LED while reading sensors and transmitting
//
// TODO: smart search through wireless networks if we cant connect
// TODO: stats output once per minute (60 seconds)
//       successful connects
//       failed connects
//       
// server side
//   C based network server
//   TCP
//   - receive inbound data
//     <MAC> <value> <value> <value>  <value>
//   - record received data with timestamp
//     <node_name> <epoch_seconds> <msg>
//   - calibration file to adjust sensors for each MAC+sensorid
//     requires that we calibrate them at 2-3 temperatures BEFORE deploying them
//   - report on missed messages based on timestamp analysis
//   - separate script to reformat for gnuplot
//   - separate script to provide overlay graphs

#include <ESP8266WiFi.h>
#include <Wire.h>

#define SERIALBAUD 9600

//---------------------------------------- WiFi Definitions
const char WiFiPSK[] = "jandgwiley";
int       svrport = 1969;

//const char WiFiSSID[] = "haus1";
// gateway interface
//IPAddress svrip(10,45,2,1); 

const char WiFiSSID[] = "haus2";
IPAddress svrip(192,168,1,2); 
// gateway interface
//IPAddress svrip(192,168,1,1); 

// initializes the wifi library, need this in file scope
WiFiClient wificlient;

// delay between reading sensors
// gets set by registration server
int       readdelay = 5000;

String macaddr;
//---------------------------------------- Pin Definitions
const int LED_PIN     = 5;  // Thing's onboard, green LED
const int ANALOG_PIN  = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read

// TC74 I2C addresses
const byte TC74A0_I2CADDR = 0b01001000;
const byte TC74A2_I2CADDR = 0b01001010;
const byte TC74A5_I2CADDR = 0b01001101;

//---------------------------------------- setup
void
setup() 
{
	int     i;
	uint8_t mac[WL_MAC_ADDR_LENGTH];

	initHardware();
	connectWiFi();

	// store our MAC address as a string for use later

	WiFi.macAddress(mac);
	for(i=0; i<WL_MAC_ADDR_LENGTH; i++)
	{
		macaddr += String(mac[i], HEX);
	}
	Serial.println(macaddr);
} // setup

//---------------------------------------- readTC74
// read values from the I2C connected TC74 temp sensor
int
readTC74(int addr)
{
	int res = 0;
	int  i;

	Wire.beginTransmission(addr);
	// 0 is the TC74 "read" command
	Wire.write(0);
	res = Wire.endTransmission();
	if(res != 0)
	{
		// blink LED/serial error message to show failed transmit
		Serial.println("write TC74 failed");
	}
	else
	{
		// we retry 3 times before giving up on the sensor
		// to give the hardware time to respond
		Wire.requestFrom(addr, 1);
		for(i=0; i<3; i++)
		{
			if(Wire.available() != 0)
			{
				res = Wire.read();
				Serial.println("read TC74 ok: " + String(res));
				break;
			}
			else
				delay(10);
		}
		if(res == 0)
			Serial.println("read TC74 failed");
	}
	
	return res;
} // readTC74

//---------------------------------------- readCT
/** read current transformer looking for the peak and return the peak
*/
int
readCT()
{
	int res = 0;
    int adc;
    unsigned long startms;
    unsigned long endms;
    unsigned long nowms;

    // if we sample for a full AC cycle we will get something
    // close to the max, 60Hz = about 15ms per cycle
    startms = millis();
    endms = startms + 20;
    while(1)
    {
		adc = analogRead(ANALOG_PIN);
        if(adc > res)
            res = adc;

        // if now is past roll over then make sure we catch it
        // if the end is past a roll over then make sure we end
        nowms = millis();
        if((startms < endms && (nowms > endms || nowms < startms))
         || startms > endms && nowms > endms)
            break;

        delay(1);
    } // while

	return res;
} // readCT

//---------------------------------------- loop
// this is the main loop
void
loop() 
{
	String msg;
	int    sensorval;

   // establish a new network connection (TCP)
	while(!wificlient.connect(svrip, svrport))
	{
		digitalWrite(LED_PIN, HIGH);
		Serial.println("failed to connect to server");
		delay(500);
		digitalWrite(LED_PIN, LOW);
	}

	digitalWrite(LED_PIN, HIGH);

	Serial.println("connected to server");

	msg = macaddr + " ";

	// TC74 at address A0

	digitalWrite(LED_PIN, HIGH);
	sensorval = readTC74(TC74A0_I2CADDR);
	msg += String(sensorval);
	msg += " ";
	digitalWrite(LED_PIN, LOW);
	delay(10);

	// TC74 at address A2

	digitalWrite(LED_PIN, HIGH);
	sensorval = readTC74(TC74A2_I2CADDR);
	msg += String(sensorval);
	msg += " ";
	digitalWrite(LED_PIN, LOW);
	delay(10);

	// TC74 at address A5

	digitalWrite(LED_PIN, HIGH);
	sensorval = readTC74(TC74A5_I2CADDR);
	msg += String(sensorval);
	msg += " ";
	digitalWrite(LED_PIN, LOW);
	delay(10);

	// CT on the AD pin

	digitalWrite(LED_PIN, HIGH);
	sensorval = readCT();
	msg += String(sensorval);
	digitalWrite(LED_PIN, LOW);
	delay(10);

	// send the readings over wifi

	digitalWrite(LED_PIN, HIGH);
	Serial.println(msg);
	wificlient.println(msg);
	wificlient.flush();
	wificlient.stop();

	delay(500);
	digitalWrite(LED_PIN, LOW);

   delay(readdelay);
	Serial.println("Client disconnected");

} // loop

//---------------------------------------- connectWiFi
// connects to the WiFi network, blink LED 100ms each attempt
void
connectWiFi()
{
   Serial.print("attempting to join WiFi SSID: ");
   Serial.println(WiFiSSID);

   digitalWrite(LED_PIN, HIGH);
   // attach to the wifi network
   WiFi.begin(WiFiSSID, WiFiPSK);
   while (WiFi.status() != WL_CONNECTED)
   {
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
   }
   digitalWrite(LED_PIN, LOW);

   Serial.println("Connected to WiFi");
} // connectWiFi

//---------------------------------------- initHardware
void
initHardware()
{
	Serial.begin(SERIALBAUD);

   Serial.println("initHardware start");

	pinMode(DIGITAL_PIN, INPUT_PULLUP);
	pinMode(ANALOG_PIN, INPUT);

	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

   // Set WiFi mode to station (as opposed to AP or AP_STA)
   WiFi.mode(WIFI_STA);
   delay(10);

	// I2C setup (as a bus master)
	Wire.begin();
	delay(10);

   Serial.println("initHardware end");
} // initHardware

// end
