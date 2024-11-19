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
const int flowSensorPin = 2; 
float totalLiters = 0.0;
volatile int pulseCount = 0;         
unsigned long lastTime = 0;   
const float calibrationFactor = 7.5;
const byte rxPin = 2;
const byte txPin = 3;

// Set up a new SoftwareSerial object
SoftwareSerial Serial1 (rxPin, txPin);

float tempCelsius; 
void setup() {
  nodemcu.begin(115200);
  BTSerial.begin(115200);
  tempSensor.begin();
  Serial.begin(115200); // Initialize serial communication
  //lcd.init(); // Initialize LCD
  //lcd.backlight(); // Turn on backlight
  //lcd.setCursor(0, 0);
  //lcd.print("TDS Value:");
  //lcd.setCursor(0,1);
  //lcd.print("Turbidity:");
  pinMode(waterLevelPin, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(flowSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulse, FALLING); // Interrupt on falling edge of signal
  pulseCount = 0;
  totalLiters = 0.0;
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

  // Limit TDS (0 to 100,000)
  if (tdsValue < 0) {
    tdsValue = 0.00;
  } else if (tdsValue > 100000) {
    tdsValue = 100000.00;
  }
  
  // Limit EC (0 to 100,000 microsiemens)
  if (ec < 0) {
    ec = 0.00;
  } else if (ec > 100000) {
    ec = 100000.00;
  }
  
  // Limit Turbidity (0 to 100 NTU)
  if (turbidity < 0) {
    turbidity = 0.00;
  } else if (turbidity > 100) {
    turbidity = 100.00;
  }
  
  // Limit pH (0.0 to 14.0)
  if (phValue < 0.0) {
    phValue = 0.00;
  } else if (phValue > 14.0) {
    phValue = 14.00;
  }
  
  // Limit Temperature (0 to 50°C, adjust for your environment)
  if (tempCelsius < 0.0) {
    tempCelsius = 0.00;
  } else if (tempCelsius > 50.0) {
    tempCelsius = 50.00;
  }
  
  // Limit Gallons (0 to 1000 gallons, adjust as needed)
  if (totalLiters < 0.00) {
    totalLiters = 0.00;
  } else if (totalLiters > 1000) {
    totalLiters = 1000.00;
  }  

  tempSensor.requestTemperatures();             // send the command to get temperatures
  tempCelsius = tempSensor.getTempCByIndex(0);  // read temperature in Celsius

  if (millis() - lastTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    totalLiters += (pulseCount * calibrationFactor) / 1000.0;
    pulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulse, FALLING);
    lastTime = millis();
  }

  bool activeTds = isSensorActive(sensorPin, false);
  bool activeTurbidity = isSensorActive(turbidityPin, false);
  bool activePh = isSensorActive(SensorPin, false);
  bool activeTemp = isSensorActive(tempPin, true);
  bool activeFlow = isSensorActive(flowSensorPin, true);
  bool sensorStatus[] = {activeTds, activeTurbidity, activePh, activeTemp, activeFlow};
  float boolArraySize = sizeof(sensorStatus) / sizeof(sensorStatus[0]);
  sendBoolArray(sensorStatus, boolArraySize);

  float dataArray[] = {tdsValue, turbidity, phValue, ec, tempCelsius, totalLiters};
  int arraySize = sizeof(dataArray) / sizeof(dataArray[0]);
  sendArray(dataArray, arraySize);
  delay(1000); // Delay for 1 second before taking the next reading
}

void sendArray(float data[], int size){
  for (int i = 0; i < size; i++) {
    byte* byteArray = (byte*) &data[i];  // Get pointer to the float's bytes
    for (int j = 0; j < 4; j++) {        // A float takes 4 bytes
      Serial.write(byteArray[j]);        // Send each byte of the float
    }
  }
}

void sendBoolArray(bool data[], int size){
  for (int i = 0; i < size; i++) {
    byte* byteArray = (byte*) &data[i];  // Get pointer to the float's bytes
    for (int j = 0; j < 4; j++) {        // A float takes 4 bytes
      nodemcu.write(byteArray[j]);        // Send each byte of the float
    }
  }
}

void countPulse() {
  pulseCount++;  
}

bool isSensorActive(int sensorPin, bool isDigital) {
  if (isDigital) {
    // For digital sensors, we check if the sensor is sending a HIGH signal
    return (digitalRead(sensorPin) == HIGH);
  } else {
    // For analog sensors, we check if the sensor value is greater than 0 (a threshold value)
    int sensorValue = analogRead(sensorPin);
    return (sensorValue > 0);  // Assuming that a valid reading is greater than 0
  }
}





