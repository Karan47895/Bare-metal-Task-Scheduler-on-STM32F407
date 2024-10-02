#include <WiFi.h>
#include <FirebaseESP32.h>  // For ESP32

// Define your WiFi credentials
#define WIFI_SSID "jaymataji"
#define WIFI_PASSWORD "karan561777"

// Firebase credentials
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

#define MAX_STRING_LENGTH 50 // Maximum length of each string
#define NUM_STRINGS 5        // Number of sensor readings to store (5 readings: light, sound, temperature, humidity, ultrasonic)

char currentString[MAX_STRING_LENGTH];            // Buffer to store the current sensor reading
char storedStrings[NUM_STRINGS][MAX_STRING_LENGTH]; // Array to store 5 sensor readings
int charIndex = 0;                                // Tracks the current character index in the string
int stringIndex = 0;                              // Tracks which sensor reading is being updated

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("UART0 Receiver Ready! Send sensor readings terminated with '\\n'");

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

  // Configure Firebase credentials
  config.host = "task-scheduler-26047-default-rtdb.asia-southeast1.firebasedatabase.app";  // Firebase Database URL (without https://)
  config.signer.tokens.legacy_token = "AIzaSyAlYV41MIJsEmbkhvykbKoD3CtvcruKRrs";  // Firebase secret

  // Connect to Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Check Firebase connection
  if (Firebase.ready()) {
    Serial.println("Firebase connection established");
  } else {
    Serial.println("Failed to connect to Firebase");
  }
}

void loop() {
  // Check if data is available on UART0 (Serial)
  if (Serial.available() > 0) {
    char incomingChar = Serial.read(); // Read the incoming character

    // Check for newline as the end of a string (sensor reading)
    if (incomingChar == '\n') {
      currentString[charIndex] = '\0'; // Null-terminate the string

      // Store the current sensor reading into the array of stored strings
      strncpy(storedStrings[stringIndex], currentString, MAX_STRING_LENGTH);

      // Print the received reading
      Serial.print("Stored Sensor Reading [");
      Serial.print(stringIndex);
      Serial.print("]: ");
      Serial.println(storedStrings[stringIndex]);

      // Move to the next sensor reading
      stringIndex++;

      // Check if we've stored all 5 sensor readings
      if (stringIndex >= NUM_STRINGS) {
        // Print all stored sensor readings
        Serial.println("\nAll 5 sensor readings received:");
        for (int i = 0; i < NUM_STRINGS; i++) {
          Serial.print("Reading ");
          Serial.print(i);
          Serial.print(": ");
          Serial.println(storedStrings[i]);
        }

        // Send all 5 sensor readings to Firebase as a structured JSON object
        sendStringsToFirebase();

        // Reset for the next set of sensor readings
        stringIndex = 0;
      }

      // Reset the buffer for the next reading
      charIndex = 0;
      memset(currentString, 0, MAX_STRING_LENGTH); // Clear the buffer
    } 
    // If it's not a newline, add the character to the current string (sensor reading)
    else {
      if (charIndex < MAX_STRING_LENGTH - 1) {
        currentString[charIndex++] = incomingChar;
      }
    }
  }
}

// Function to send sensor readings to Firebase as a structured JSON object
void sendStringsToFirebase() {
  // Create a FirebaseJson object
  FirebaseJson json;

  // Add each sensor reading to the FirebaseJson object with appropriate names
  json.set("Light sensor", storedStrings[0]);
  json.set("Sound sensor", storedStrings[1]);
  json.set("Temperature", storedStrings[2]);
  json.set("Humidity", storedStrings[3]);
  json.set("ultrasonic_sensor", storedStrings[4]);

  String path = "/sensorReadings";  // Path where the data will be stored

  // Send the structured JSON object to Firebase
  if (Firebase.setJSON(firebaseData, path, json)) {
    // Prepare a String to hold the JSON output
    String jsonString;
    // Get the JSON string representation
    json.toString(jsonString, false);  // Second parameter indicates whether to prettify or not

    Serial.println("Successfully sent all sensor readings to Firebase:");
    Serial.println(jsonString);  // Print the JSON string representation
  } else {
    Serial.print("Failed to send sensor readings to Firebase. Error: ");
    Serial.println(firebaseData.errorReason());
  }
}
