/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ESP8266WiFi.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#define ssid      "opendata"
#define password  "opendata"
/*
#define ssid      "Lab206_Wi-Fi"
#define password  "buck1234"
*/
/*
#define ssid      "age"
#define password  "avatar5413"
*/

#define PHm           3.5
#define PHc           -0.7
          
#define VREF          5.0
#define ADC_MAX       1024

#define host      "api.thingspeak.com"
#define httpPort  80

float ph, temp;
float x;
int   ad_read;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int count = 0;

float R1 = 1000;    //焊接的固定電阻, 必須大於300
#define Ra  25      //Resistance of powering Pins
#define ECPin   A0
#define ECGround  3
#define ECPower   1

#define PPMconversion   0.7
#define TemperatureCoef 0.019
#define K               2.88

float Temperature=10;
float EC=0;
float EC25 =0;
int ppm =0;

float raw= 0;
float Vin= 3.3;
float Vdrop= 0;
float Rc= 0;
float buffer=0;

void setup() {
  Serial.begin(115200);

  pinMode(ECPin,INPUT);
  pinMode(ECPower,OUTPUT);//Setting pin for sourcing current
  pinMode(ECGround,OUTPUT);//setting pin for sinking current
  digitalWrite(ECGround,LOW);//We can leave the ground connected permanantly

  sensors.begin();

  //** Adding Digital Pin Resistance to [25 ohm] to the static Resistor *********//
  // Consule Read-Me for Why, or just accept it as true
  R1=(R1+Ra);// Taking into acount Powering Pin Resitance

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(1000);

  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  count++;
  if(count < 3)
    return;

  sensors.requestTemperatures(); // Send the command to get temperatures
  temp = sensors.getTempCByIndex(0);

  ad_read = analogRead(A0);
  x=(ad_read * VREF) / ADC_MAX;
  ph=PHm * x + PHc;

  delay(1000);

  GetEC();

  // This will send the request to the server
  client.print("GET /update?key=29CVWTPVZYN13KGW");

  client.print("&field1=");
  client.print(temp);
  client.print("&field2=");
  client.print(ph);
  client.print("&field3=");
  client.print(EC25);

  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\n"); 
  client.print("Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");

  count=0;
}

//************ This Loop Is called From Main Loop************************//
void GetEC(){

//*********Reading Temperature Of Solution *******************//
Temperature=temp; //Stores Value in Variable

//************Estimates Resistance of Liquid ****************//
digitalWrite(ECPower,HIGH);
raw= analogRead(ECPin);
raw= analogRead(ECPin);// This is not a mistake, First reading will be low beause if charged a capacitor
digitalWrite(ECPower,LOW);

//***************** Converts to EC **************************//
Vdrop= (Vin*raw)/1024.0;
Rc=(Vdrop*R1)/(Vin-Vdrop);
Rc=Rc-Ra; //acounting for Digital Pin Resitance
EC = 1000/(Rc*K);

//*************Compensating For Temperaure********************//
EC25  =  EC/ (1+ TemperatureCoef*(Temperature-25.0));
ppm=(EC25)*(PPMconversion*1000);
}
//************************** End OF EC Function ***************************//

