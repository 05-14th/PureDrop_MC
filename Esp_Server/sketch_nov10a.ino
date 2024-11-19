#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>  
#include <TimeLib.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>

#define API_KEY "AIzaSyB-WxMTq66o_Rt4Jd2G3kYSgPbQNr0iF7I"

#define USER_EMAIL "thisyourman106@gmail.com"
#define USER_PASSWORD "kamusari24"
#define DATABASE_URL "https://puredrop-d420e-default-rtdb.firebaseio.com"
#define RELAY_PIN D7
#define INDICATOR D6

void authHandler();

void printError(int code, const String &msg);

void printResult(AsyncResult &aResult);

DefaultNetwork network; 

void timeStatusCB(uint32_t &ts);

String genUUID();

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;
AsyncResult aResult_no_callback;
SoftwareSerial arduino(D1, D2); // RX | TX
bool isRunning = false;
const long  gmtOffset_sec = 8 * 3600;

WiFiUDP udp;
NTPClient timeClient(udp, "time.google.com", gmtOffset_sec); 
unsigned long ms = 0;

void setup()
{
    Serial.begin(115200);
    arduino.begin(115200);
    WiFiManager wifiManager;
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(INDICATOR, OUTPUT);

    if (!wifiManager.startConfigPortal("PureDrop Model 0", "okaylangyan3")) {  // "ESP8266-Setup" is the AP name
      Serial.println("Failed to connect to Wi-Fi. Restarting...");
      delay(3000);
      ESP.restart();  
    }

    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); 
    digitalWrite(INDICATOR, HIGH);

    // Initialize NTP client
    timeClient.begin();


    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

    ssl_client.setInsecure();
    ssl_client.setBufferSizes(4096, 1024);

    initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);

    authHandler();

    // Binding the FirebaseApp for authentication handler.
    // To unbind, use Database.resetApp();
    app.getApp<RealtimeDatabase>(Database);

    Database.url(DATABASE_URL);

    // In case setting the external async result to the sync task (optional)
    // To unset, use unsetAsyncResult().
    aClient.setAsyncResult(aResult_no_callback);
    digitalWrite(RELAY_PIN, HIGH);
    setNotification(true);
}

void loop()
{
    if (timeClient.update()) {
        Serial.println("Time synced successfully");
    } else {
        Serial.println("Failed to sync time");
    }
    authHandler();
    getButtonState();
    updateStatus();
    pushData();
    delay(1000);
    Database.loop();
}

void authHandler()
{
    // Blocking authentication handler with timeout
    unsigned long ms = millis();
    while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
    {
        // The JWT token processor required for ServiceAuth and CustomAuth authentications.
        // JWT is a static object of JWTClass and it s not thread safe.
        // In multi-threaded operations (multi-FirebaseApp), you have to define JWTClass for each FirebaseApp,
        // and set it to the FirebaseApp via FirebaseApp::setJWTProcessor(<JWTClass>), before calling initializeApp.
        JWT.loop(app.getAuth());
        printResult(aResult_no_callback);
    }
}

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }
}

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}

void pushData(){
  if (isRunning) {
    if (Serial.available() > 0) {
    int arraySize = Serial.read();  // Read the array size (1 byte)
    float data_[arraySize];  // Array to store received floats

    // Receive each float (4 bytes per float)
    for (int i = 0; i < arraySize; i++) {
      byte byteArray[4];  // To store 4 bytes of each float
      for (int j = 0; j < 4; j++) {
        byteArray[j] = Serial.read();  // Read 4 bytes for the float
      }
      // Convert the bytes to float
      float receivedValue;
      memcpy(&receivedValue, byteArray, sizeof(receivedValue));  // Convert bytes to float
      data_[i] = receivedValue;  // Store the received float
      }

    StaticJsonDocument<200> doc;
    // Initial sync
    unsigned long epochTime = timeClient.getEpochTime();
    time_t rawTime = (time_t)epochTime;
    struct tm* ptm= localtime(&rawTime);
    int year = ptm->tm_year+1900;
    int month =  ptm->tm_mon+1;
    int day = ptm->tm_mday;
    int hour = ptm->tm_hour;
    int minute = ptm->tm_min;
    int second = ptm->tm_sec; // Calculate the second
    unsigned long currentMillis = millis();
    int milliseconds = currentMillis % 1000; 


    String monthStr = (month < 10) ? "0" + String(month) : String(month);
    String dayStr = (day < 10) ? "0" + String(day) : String(day);
    String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
    String minuteStr = (minute < 10) ? "0" + String(minute) : String(minute);
    String secondStr = (second < 10) ? "0" + String(second) : String(second);

    String Timestamp = hourStr + ":" + minuteStr + ":" + secondStr;
    String formatted_time = dayStr + "_" +  Timestamp;
    // Construct the JSON string to store data
    String jsonString = "{\"EC\":\"" + String(data_[3]) + 
                        "\",\"Gallons\":\"" + String(data_[5]) + 
                        "\",\"TDS\":\"" + String(data_[0]) + 
                        "\",\"Temperature\":\"" + String(data_[4]) + 
                        "\",\"Time\":\"" + String(Timestamp) + 
                        "\",\"Turbidity\":\"" + String(data_[1]) + 
                        "\",\"pH Level\":\"" + String(data_[2]) + "\"}";

    object_t obj(jsonString);

    // Generate a unique key by concatenating formatted_time with millis or a random number
    String uniqueKey = formatted_time;  // Ensures uniqueness using millis

    // Generate the Firebase path using the unique key
    String path = "/your_data_node/" + String(year) + "/" + String(monthStr) + "/";
    String dataPath = path + uniqueKey;  // Custom key that ensures each entry is unique

    // Use set() to store data at the unique key path (without overwriting)
    bool status = Database.set<object_t>(aClient, dataPath, obj);

    if (aClient.lastError().code() == 0) {
      Firebase.printf("Data added successfully at %s\n", dataPath.c_str());
    } else {
      printError(aClient.lastError().code(), aClient.lastError().message());
    }
    }
  }
}

void updateStatus(){
  if (arduino.available() > 0) {
    int arraySize = arduino.read();  // Read the array size (1 byte)
    float data_[arraySize];  // Array to store received floats

    // Receive each float (4 bytes per float)
    for (int i = 0; i < arraySize; i++) {
      byte byteArray[4];  // To store 4 bytes of each float
      for (int j = 0; j < 4; j++) {
        byteArray[j] = arduino.read();  // Read 4 bytes for the float
      }
      // Convert the bytes to float
      float receivedValue;
      memcpy(&receivedValue, byteArray, sizeof(receivedValue));  // Convert bytes to float
      data_[i] = receivedValue;  // Store the received float

      String data = "{\n";
      data += "  \"TDS\": \"" + String(data_[0]) + "\",\n";
      data += "  \"Turbidity\": \"" + String(data_[1]) + "\",\n";
      data += "  \"pH Level\": \"" + String(data_[2]) + "\",\n";
      data += "  \"Temperature\": \"" + String(data_[3]) + "\",\n";
      data += "  \"Gallon\": \"" + String(data_[4]) + " L\"\n";
      data += "}";

      arduino.println(data);
    }
  }
}

void getButtonState(){
  Serial.print("Getting Button State... ");
  String button_state = Database.get<String>(aClient, "/switch_button");
  Serial.println(button_state);
  if (aClient.lastError().code() == 0)
      if(button_state == "on"){
        digitalWrite(RELAY_PIN, HIGH);
        isRunning = true;
      }else{
        digitalWrite(RELAY_PIN, LOW);
        isRunning = false;
      }
  else
      printError(aClient.lastError().code(), aClient.lastError().message());
}

void setNotification(bool state){
  Database.set<bool>(aClient, "/notification", state);
}

void updateHealth(bool state, String sensorName){
  Database.set<bool>(aClient, "/Health/" + sensorName, state);
}

void timeStatusCB(uint32_t &ts)
{
#if defined(ESP8266) || defined(ESP32) || defined(CORE_ARDUINO_PICO)
    if (time(nullptr) < FIREBASE_DEFAULT_TS)
    {

        configTime(3 * 3600, 0, "pool.ntp.org");
        while (time(nullptr) < FIREBASE_DEFAULT_TS)
        {
            delay(100);
        }
    }
    ts = time(nullptr);
#elif __has_include(<WiFiNINA.h>) || __has_include(<WiFi101.h>)
    ts = WiFi.getTime();
#endif
}

String genUUID()
{
    // This is how Firebase generate UUID when you calling Push.

    // Push UUID generator, https://gist.github.com/mikelehen/3596a30bd69384624c11

    /**
     * Fancy ID generator that creates 20-character string identifiers with the following properties:
     *
     * 1. They're based on timestamp so that they sort *after* any existing ids.
     * 2. They contain 72-bits of random data after the timestamp so that IDs won't collide with other clients' IDs.
     * 3. They sort *lexicographically* (so the timestamp is converted to characters that will sort properly).
     * 4. They're monotonically increasing.  Even if you generate more than one in the same timestamp, the
     *    latter ones will sort after the former ones.  We do this by using the previous random bits
     *    but "incrementing" them by 1 (only in the case of a timestamp collision).
     */

    // Modeled after base64 web-safe chars, but ordered by ASCII.
    static char PUSH_CHARS[] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

    // Timestamp of last push, used to prevent local collisions if you push twice in one ms.
    static long long lastPushTime = 0;

    // We generate 72-bits of randomness which get turned into 12 characters and appended to the
    // timestamp to prevent collisions with other clients.  We store the last characters we
    // generated because in the event of a collision, we'll use those same characters except
    // "incremented" by one.
    char lastRandChars[72] = "";
    char timeStampChars[9] = "";

    uint32_t ts = 0;
    timeStatusCB(ts);

    long long now = ts * 1000LL;

    srand(now);

    bool duplicateTime = (now == lastPushTime);
    lastPushTime = now;

    for (int i = 7; i >= 0; i--)
    {
        timeStampChars[i] = PUSH_CHARS[(int)(now % 64)];
        now = now / 64;
    }

    // We should have converted the entire timestamp.
    if (now != 0)
        return String();

    timeStampChars[8] = '\0';

    String id = timeStampChars;

    if (!duplicateTime)
    {
        for (int i = 0; i < 12; i++)
        {
            double fl = ((double)rand() / (double)(RAND_MAX + 1.0)) * 64;
            lastRandChars[i] = (char)floor(fl);
        }
    }
    else
    {
        // If the timestamp hasn't changed since last push, use the same random number, except incremented by 1.
        int val = 0;
        for (int i = 11; i >= 0 && lastRandChars[i] == 63; i--)
        {
            val = i;
            lastRandChars[i] = 0;
        }

        if (val >= 0)
            lastRandChars[val]++;
    }

    for (int i = 0; i < 12; i++)
        id += PUSH_CHARS[(int)lastRandChars[i]];

    // Length should be 20.
    if (id.length() != 20)
        return String();

    return id;
}


