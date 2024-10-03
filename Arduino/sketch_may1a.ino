#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PH_OFFSET -2.54 // Offset value based on calibration
#define SensorPin A2      // Analog pin connected to the pH sensor
unsigned long int avgValue;  // Store the average value of the sensor feedback
float b;
SoftwareSerial nodemcu(5, 6);
SoftwareSerial BTSerial(2, 3); // RX | TX

LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16 columns, 2 rows
const int sensorPin = A0; // Analog pin connected to the sensor
const int turbidityPin = A1;     
//const int tempPin = A3;
const int waterLevelPin = 7;
const int tempPin = 13;
int buf[10],temp;
float voltage = 0; // Variable to store the voltage
float tdsValue = 0; // Variable to store the TDS value
float volt;
float ntu;
double ec;
String str;
OneWire oneWire(tempPin);         // setup a oneWire instance
DallasTemperature tempSensor(&oneWire);
const int RELAY_PIN = 12;

float tempCelsius; 
void setup() {
  nodemcu.begin(9600);
  BTSerial.begin(9600);
  tempSensor.begin();
  Serial.begin(9600); // Initialize serial communication
  //lcd.init(); // Initialize LCD
  //lcd.backlight(); // Turn on backlight
  //lcd.setCursor(0, 0);
  //lcd.print("TDS Value:");
  //lcd.setCursor(0,1);
  //lcd.print("Turbidity:");
  pinMode(waterLevelPin, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
  
  
  // Read the analog voltage from the sensor
  voltage = analogRead(sensorPin) * 5.0 / 1024.0; // Convert the analog reading to voltage
  // Convert the voltage to TDS value
  tdsValue = voltage * 1000.0 / 5.0; // Assuming a linear relationship between voltage and TDS
  // Print the TDS value to the LCD
  //lcd.setCursor(11, 0);
  //lcd.print("          "); // Clear the previous reading
  //lcd.setCursor(11, 0);
  //lcd.print(tdsValue); // Print the TDS value
  
  //Serial.print("TDS Value: ");
  //Serial.print(tdsValue);
  //Serial.println(" ppm");

  int turbidityValue = analogRead(turbidityPin);
  int turbidity = map(turbidityValue, 0,750, 100, 0);
  //turbidity = turbidity * -1;
  //delay(100);
  //lcd.setCursor(11, 1);
  //lcd.print("   ");
  //lcd.setCursor(11, 1);
  
  //lcd.print(turbidity);

  
  /*if (turbidity > 15) {
    //lcd.setCursor(0, 1);
    //lcd.print(" its CLEAR ");
  }
  if ((turbidity < 15) && (turbidity > 0)) {
    //lcd.setCursor(0, 1);
    //lcd.print(" its CLOUDY ");
  }
  if (turbidity < 0) {
    //lcd.setCursor(0, 1);
    //lcd.print(" its DIRTY ");
  }*/
  //turbidity = 0;

  for(int i=0; i<10; i++) {       // Get 10 sample values from the sensor for smoothing
    buf[i] = analogRead(SensorPin);
    delay(10);
  }
  
  for(int i=0; i<9; i++) {        // Sort the analog values from small to large
    for(int j=i+1; j<10; j++) {
      if(buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  avgValue = 0;
  for(int i=2; i<8; i++) {        // Take the average value of the 6 center samples
    avgValue += buf[i];
  }

  float phValue = (float)avgValue * 5.0 / 1024 / 6; // Convert the analog value into millivolts
  phValue = 3.5 * phValue;          // Convert the millivolts into pH value
  phValue = phValue + PH_OFFSET;    // Apply calibration offset
  //str =String("coming from arduino: ")+String("TDS= ")+String(tdsValue)+" "+String("NTU= ")+String(turbidity)+" "+String("PH= ")+String(phValue);

  ec = (tdsValue * 2/1000) / 10;

  tempSensor.requestTemperatures();             // send the command to get temperatures
  tempCelsius = tempSensor.getTempCByIndex(0);  // read temperature in Celsius

  String dataString = String(tdsValue) + ","+ String(ec)+ "," + String(turbidity) + "," + String(phValue) + "," + String(tempCelsius) + "°C";
  Serial.println(dataString);
  delay(1000); // Delay for 1 second before taking the next reading
}








