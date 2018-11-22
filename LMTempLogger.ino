#include <WiFi.h>
#include <SPI.h>



char ssid[] = "";      //  your network SSID (name) 
char pass[] = "";   // your network passwordclient.availableclient.available

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
char server[] = "";
int deviceID = 1;

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;
int updateingTemp = 0;

/////////////////////////////
char *ok_dtostrf (double val, signed char width, unsigned char prec, char *sout);
float tempraturepCelsius(int rawvoltage);
//char tempString[10] ;
unsigned long updateIntervel = (60000 * 60);
#define analog_pin 0 // we tie 3.3V to ARef and measure it with a multimeter!
int led = 13;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 

  String fv = WiFi.firmwareVersion();
  if( fv != "1.1.0" )
    Serial.println("Please upgrade the firmware");

  // attempt to connect to Wifi network:
  connectToWiFi ();

  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
   pinMode(led, OUTPUT);



}

void loop() {
  
  int rawvoltage = analogRead(analog_pin);
  float cTemp = tempraturepCelsius (rawvoltage);
  String tempString = "";     
  
  tempString += String(int(cTemp))+ "."+String(getDecimal(cTemp));
//  ok_dtostrf(cTemp, 6, 2, tempString);

  Serial.print("====== Celsius : ");
  Serial.print(tempString);
  Serial.print(" =========");
  Serial.println();

  unsigned long startTime = millis();
  system("date");
  digitalWrite(led, HIGH);
  sendTempratureRequest(tempString);

  // if there are incoming bytes available 
  // from the server, read them and print them:
  int readingJSON = 0;
  String jsonResp = "";
  while (client.available()) {

    char c = client.read();
    if(c == '{' && readingJSON == 0) {
      readingJSON = 1;
    }
    
    
    if(c == -1 && readingJSON == 1) {
      readingJSON = 0;
    }
    
    if (readingJSON == 1) {

      jsonResp = jsonResp + c;
    }
//    Serial.print(c);
  } 

  Serial.println("\n=====JSON=====\n");
  Serial.println(jsonResp);

  if(!client.available()){
    Serial.println();
    Serial.println("\n========= Request Completed =========\n");
    updateingTemp = 0;
    client.flush();
    Serial.println();
    Serial.println("\ndisconnecting from server.\n");
    client.stop();
  digitalWrite(led, LOW);
  }

  unsigned long endTime = millis();
  unsigned long spend = endTime - startTime;

  Serial.println();
  Serial.print("spend : ");
  Serial.print(spend);
  Serial.println();

  if(spend > updateIntervel){
    delay(updateIntervel); 
  } 
  else {

    delay((updateIntervel - spend)); 
  }


}


void connectToWiFi () {

  while (status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID : ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  } 

}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void sendTestRequest() {
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET /test HTTP/1.1");
    //    client.println("User-agent: Intel-Edison");
    client.println("Host: api.smillingpari.com");
    client.println("User-agent: intel_edison");
    client.println("Connection: close");
    client.println();
  }
}

void sendTempratureRequest(String readValue) {

  int _status = WiFi.status();
  if (_status != WL_CONNECTED) {
    connectToWiFi ();
  }

  Serial.println("Sending Request");
  if (client.connect(server, 80)) {

    String stringToPost = "?sensor_id";

    stringToPost = stringToPost + "=" + 1;
    stringToPost = stringToPost + "&";

    stringToPost = stringToPost + "sensor_data" + "=" + readValue;
    stringToPost = stringToPost + "&";

    stringToPost = stringToPost + "sensor_state" + "=" + 1;
    stringToPost = stringToPost + "&";

    stringToPost = stringToPost + "sensor_data_type" + "=" + "temp" + ":" + "c";
    Serial.print("body: ");
    Serial.println(stringToPost);
    //    Serial.print("Content-Length: ");
    //    Serial.println(stringToPost.length());

    Serial.println("connected to server");
    // Make a HTTP request:
    String postReq = "GET /iot/record" + stringToPost;
    postReq = postReq + " HTTP/1.1";
    client.println(postReq);
    //    Serial.println(postReq);
    client.println("Host: api.smillingpari.com");
    client.println("User-Agent: intel_edison");
    client.println("Device-Key: SMP_547b37ca6ce78");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println();
    //    client.print("Content-Length: ");
    //    client.println(stringToPost.length());
    //    client.println("\n\n");
    //    client.print(stringToPost);
    updateingTemp = 1;
    delay(10000);

  } 
  else {

    Serial.println("Unable to connect");
  }
}
//Helper
char *ok_dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[32];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}

long getDecimal(float val)
{
int intPart = int(val);
long decPart = 100*(val-intPart); //I am multiplying by 100 assuming that the foat values will have a maximum of 3 decimal places
                                   //Change to match the number of decimal places you need
if(decPart>0)return(decPart);           //return the decimal part of float number if it is available
else if(decPart<0)return((-1)*decPart); //if negative, multiply by -1
else if(decPart=0)return(00);           //return 0 if decimal part of float number is not available
}

//temprature calculation

float tempraturepCelsius(int rawvoltage) {
  float millivolts = (rawvoltage/1023.0) * 3450;
  float celsius = millivolts/10;
  return celsius;
}



