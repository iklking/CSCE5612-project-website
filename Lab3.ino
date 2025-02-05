#include <Arduino.h> // Library for 
#include <U8x8lib.h> // Library for OLED
#include "LSM6DS3.h" // Library for IMU
#include <Wire.h> // Library for IMU
#include <ArduinoBLE.h> // Library for Bluetooth
#include <PCF8563.h> // Library for RTC
#include <SPI.h> // Library for MicroSD
#include <SD.h> // Library for MicroSD
#include <PDM.h> //Library for Microphone

// Variables for MicroSD
const int chipSelect = 2;

// Variables for RTC
PCF8563 pcf;

// Variables for OLED display
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* clock=*/ PIN_WIRE_SCL, /* data=*/
PIN_WIRE_SDA, /* reset=*/ U8X8_PIN_NONE);

// Variables for voltage
const float R1 = 2880.0;  // 2.88kΩ resistor
const float R2 = 10000.0;  // 10kΩ resistor
const float referenceVoltage = 3.3; // Reference voltage of the ADC (usually 5V or 3.3V)

// Variables for Bluetooth
BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite); // Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central

// Variable for LED
const int ledPin = LED_BUILTIN; // pin to use for the LED

// Variable for IMU: Accelerometer, Gyroscope, Thermometer
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// Varables for microphone/PDM
static const char channels = 1; // Default number of output channels
static const int frequency = 16000; // Default PCM output frequency
short sampleBuffer[512]; // Buffer to read samples into, each sample is 16-bits
volatile int samplesRead; // Number of audio samples read

void setup() {
  // Initialize OLED display
  u8x8.begin();
  u8x8.setFlipMode(1);

  Serial.begin(9600); // Initialize serial communication at 9600 bits per second:

  //while (!Serial);

  // Set up LED
  pinMode(ledPin, OUTPUT); // Set LED pin to output mode

  // Begin initialization of Bluetooth
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }
  BLE.setLocalName("Kaylyn"); // set advertised local name and service UUID
  BLE.setAdvertisedService(ledService);
  ledService.addCharacteristic(switchCharacteristic); // add the characteristic to the service
  BLE.addService(ledService); // add service
  switchCharacteristic.writeValue(0); // set the initial value for the characeristic:
  BLE.advertise(); // start advertising

  // Intialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("Device error");
  } else {
    Serial.println("Device OK!");
  }

  // Initialize clock
  pcf.init(); //initialize the clock
  pcf.stopClock(); //stop the clock
  pcf.startClock(); //start the clock

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
  }

  // Initialize microphone
  PDM.onReceive(onPDMdata); // Configure the data receive callback
  if (!PDM.begin(channels, frequency)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
}

void loop() {

  // Calculate Voltage
  int sensorValue = analogRead(A0); // Read the input on analog pin A0
  float measuredVoltage = sensorValue * (referenceVoltage / 1023.0); // Convert the ADC value to voltage
  float batteryVoltage = measuredVoltage * ((R1 + R2) / R2); // Account for the voltage divider to get the actual battery voltage
  // Display Battery Voltage
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0,0);
  u8x8.print("Battery Voltage:");
  u8x8.setCursor(0,1);
  u8x8.print(batteryVoltage);
  u8x8.print(" V");

  if(batteryVoltage > 3.2){ // Only log when voltage is above 3.2 V

    BLEDevice central = BLE.central();

    u8x8.setCursor(0,2);
    u8x8.print("System: ");

    if (switchCharacteristic.value()) {   // any value other than 0
      u8x8.println("ON");         // Display "ON"
      digitalWrite(ledPin, LOW);  // will turn the LED on


      String dataString = ""; // make a string for assembling the data to log

      Time nowTime = pcf.getTime();// get current time

      // Add data from IMU and RTC to data string to log
      dataString += "Date: " + String(nowTime.day) + "/" + String(nowTime.month) + "/" + String(nowTime.year) + " " + String(nowTime.hour) + ":" + String(nowTime.minute) + ":" + String(nowTime.second) + "\n";
      dataString += "Accelerometer:\n X1 = " + String(myIMU.readFloatAccelX()) + "\n Y1 = " + String(myIMU.readFloatAccelY()) + "\n Z1 = " + String(myIMU.readFloatAccelZ()) + "\n";
      dataString += "Gyroscope:\n X1 = " + String(myIMU.readFloatGyroX()) + "\n Y1 = " + String(myIMU.readFloatGyroY()) + "\n Z1 = " + String(myIMU.readFloatGyroZ()) + "\n";
      dataString += "Thermometer:\n Degrees C1 = " + String(myIMU.readTempC()) + "\n Degrees F1 = " + String(myIMU.readTempF()) + "\n";
      dataString += "Microphone:\n";
      if (samplesRead) {
        // Print samples to the serial monitor or plotter
        for (int i = 0; i < samplesRead; i++) {
          if(channels == 2) {
            dataString += " L: " + String(sampleBuffer[i]) + "\n R: ";
            i++;
          }
          dataString += String(sampleBuffer[i]) + "\n";
        }
        // Clear the read count
        samplesRead = 0;
      }

      File dataFile = SD.open("datalog.txt", FILE_WRITE); // open the file

      if (dataFile) { // if the file is available, write to it
        dataFile.println(dataString);
        dataFile.close();
      }
      else { // if the file isn't open, pop up an error:
        Serial.println("error opening datalog.txt");
      }

    } else { // a 0 value
      u8x8.println("OFF"); // Displays "OFF"         
      digitalWrite(ledPin, HIGH); // will turn the LED off
    }

    delay(1000); // Delay in between reads for stability

  } else { // output low battery when below 3.2V and stop logging
    u8x8.setCursor(0,2);
    u8x8.println("Low Battery!");
  }

}

// Callback function to process the data from the PDM microphone.
void onPDMdata() {
  // Query the number of available bytes
  int bytesAvailable = PDM.available();

  // Read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
