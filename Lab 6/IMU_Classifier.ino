#include <LSM6DS3.h>
#include <Wire.h>
#include <PCF8553.h>
#include <SPI.h>
#include <SD.h>

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
//#include <tensorflow/lite/version.h>

#include "model.h"

PCF8563 pcf;

const float accelerationThreshold = 2.5; // threshold of significant in G's
const int numSamples = 119;

const int chipSelect = 10;

int samplesRead = numSamples;

LSM6DS3 myIMU(I2C_MODE, 0x6A);  

// global variables used for TensorFlow Lite (Micro)
tflite::MicroErrorReporter tflErrorReporter;

// pull in all the TFLM ops, you can remove this line and
// only pull in the TFLM ops you need, if would like to reduce
// the compiled size of the sketch.
tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Create a static memory buffer for TFLM, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

// array to map gesture index to a name
const char* GESTURES[] = {
  "sitting",
  "standing",
  "walking",
  "ascending",
  "descending"
};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

void setup() {

  Serial.begin(9600);
  pcf.init();//initialize the clock
  pcf.stopClock();//stop the clock
  //set time to to 31/3/2018 17:33:0
  pcf.setYear(25);//set year
  pcf.setMonth(2);//set month
  pcf.setDay(27);//set dat
  pcf.setHour(17);//set hour
  pcf.setMinut(33);//set minut
  pcf.setSecond(0);//set second
  pcf.startClock();//start the clock

  Serial.begin(9600);
  while (!Serial);

  if (myIMU.begin() != 0) {
    Serial.println("Device error");
  } else {
    Serial.println("Device OK!");
  }

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
  }

  Serial.println();

  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // make a string for assembling the data to log:
  String dataString = "";

  // wait for significant motion
  while (samplesRead == numSamples) {
      // read the acceleration data
      aX = myIMU.readFloatAccelX();
      aY = myIMU.readFloatAccelY();
      aZ = myIMU.readFloatAccelZ();

      // sum up the absolutes
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // check if it's above the threshold
      if (aSum >= accelerationThreshold) {
        // reset the sample read count
        samplesRead = 0;
        break;
      }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {

    Time nowTime = pcf.getTime();//get current time

    // check if new acceleration AND gyroscope data is available
    // read the acceleration and gyroscope data
      
    aX = myIMU.readFloatAccelX();
    aY = myIMU.readFloatAccelY();
    aZ = myIMU.readFloatAccelZ();

    //gX = myIMU.readFloatGyroX();
    //gY = myIMU.readFloatGyroY();
    //gZ = myIMU.readFloatGyroZ();

    // normalize the IMU data between 0 to 1 and store in the model's
    // input tensor
    tflInputTensor->data.f[samplesRead * 6 + 0] = (aX + 4.0) / 8.0;
    tflInputTensor->data.f[samplesRead * 6 + 1] = (aY + 4.0) / 8.0;
    tflInputTensor->data.f[samplesRead * 6 + 2] = (aZ + 4.0) / 8.0;
    //tflInputTensor->data.f[samplesRead * 6 + 3] = (gX + 2000.0) / 4000.0;
    //tflInputTensor->data.f[samplesRead * 6 + 4] = (gY + 2000.0) / 4000.0;
    //tflInputTensor->data.f[samplesRead * 6 + 5] = (gZ + 2000.0) / 4000.0;

    samplesRead++;

    if (samplesRead == numSamples) {
      // Run inferencing
      TfLiteStatus invokeStatus = tflInterpreter->Invoke();
      if (invokeStatus != kTfLiteOk) {
        Serial.println("Invoke failed!");
        while (1);
        return;
      }

      // Loop through the output tensor values from the model
      for (int i = 0; i < NUM_GESTURES; i++) {
        //print current time
        dataString += String(nowTime.hour + ":" + nowTime.minute + ":" + nowTime.second + ", ");;  
        dataString += String(GESTURES[i]);
      }
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
        // print to the serial port too:
        Serial.println(dataString);
      } else {
        Serial.println("error opening datalog.txt");
      }
    }
  }
}
