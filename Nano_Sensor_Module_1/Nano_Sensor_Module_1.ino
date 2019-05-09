#include <TransferI2C_WLC.h>

#include <Wire.h>

//Sump Sensor Module

//I2C communication address
int I2C_SUMP_MODULE_ADDRESS = 11;
int I2C_TANK_MODULE_ADDRESS = 12;

//overhead Tank Sensor
//Tank1 sensors Pins
const int Tank1TrigPin1 = 2;
const int Tank1EchoPin1 = 3;

//Tank1 backup sensors Pins
const int Tank1TrigPin2 = 4;
const int Tank1EchoPin2 = 5;

//Tank2 sensors Pins
const int Tank2TrigPin1 = 6;
const int Tank2EchoPin1 = 7;

//Tank2 backup sensors Pins
const int Tank2TrigPin2 = 8;
const int Tank2EchoPin2 = 9;

//LED Pins
const int LED1 = A6;

const int MaxTickCount  = 5;
const int SensorsMaxCount = 2;
int MaxTankCount = 1;

const String logFunc = "Nano-Sensor SumpModule";
//Error reading for valus in inches
const int ErrorReading = 1000;
bool EnableDebug = true;

struct TANK_DATA_STRUCTURE {
  int tankNo;
  float sensorValue;
};

//Config data structure
struct CONFIG_DATA_STRUCTURE {
  int TotalTanks;
};

//create object
TANK_DATA_STRUCTURE Tank_Data;
CONFIG_DATA_STRUCTURE Config_Data;

TransferI2C_WLC TransferOut, TransferIn;

void setup() {

  //LED Display
  pinMode(LED1, OUTPUT);

  //UltaSound Sensor pin for Overhead Tank1
  pinMode(Tank1TrigPin1, OUTPUT); // Sets the trigPin as an Output
  pinMode(Tank1EchoPin1, INPUT); // Sets the echoPin as an Input
  pinMode(Tank1TrigPin2, OUTPUT); // Sets the trigPin as an Output
  pinMode(Tank1EchoPin2, INPUT); // Sets the echoPin as an Input

  //UltaSound Sensor pin for Overhead Tank2
  pinMode(Tank2TrigPin1, OUTPUT); // Sets the trigPin as an Output
  pinMode(Tank2EchoPin1, INPUT); // Sets the echoPin as an Input
  pinMode(Tank2TrigPin2, OUTPUT); // Sets the trigPin as an Output
  pinMode(Tank2EchoPin2, INPUT); // Sets the echoPin as an Input

  //Begin wire transmission to slave(Aurdino UNO Module)
  Wire.begin(I2C_SUMP_MODULE_ADDRESS); // join i2c bus (address optional for master)
  Wire.onRequest(request);

  //Logging
  Serial.begin(9600); // Starts the serial communication

  TransferOut.begin(details(Tank_Data), &Wire);  //this initializes the Tank_data data object
  TransferIn.begin(details(Config_Data), &Wire);  //this initializes the Config_data data object

  //define handler function on receiving data
  Wire.onReceive(receive);
}

void receive(int numBytes) {}

void request() {

  LogSerial(false, logFunc, true, "Tanks Selected : ");
  LogSerial(false, logFunc, true, String(Tank_Data.tankNo));
  LogSerial(false, logFunc, true, " Distance : ");
  LogSerial(true, logFunc, true, String(Tank_Data.sensorValue));

  //Transfer data to Master/Main Controller
  TransferOut.flagSlaveSend();
  TransferOut.sendData();

}

void loop() {

  for (int tank = 1; tank <= MaxTankCount ; tank++)
  {
    float distance = GetTankStatus(tank);

    Tank_Data.tankNo = 1;
    Tank_Data.sensorValue = 2;
  }

  delay(300);
}

//Get Tank Status for selected tankNo
float GetTankStatus(int tankNo)
{
  const String logFunc = "GetTankStatus()";

  int trigPin1 = 0;
  int echoPin1 = 0;
  int trigPin2 = 0;
  int echoPin2 = 0;
  String tank1Pins = "";

  switch (tankNo)
  {
    case 1:
      trigPin1 = Tank1TrigPin1;
      echoPin1 = Tank1EchoPin1;
      trigPin2 = Tank1TrigPin2;
      echoPin2 = Tank1EchoPin2;
      //Log pin nos of tank1

      tank1Pins = "trigPin1:";
      tank1Pins += String(trigPin1);
      tank1Pins += ",echoPin1:";
      tank1Pins += String(echoPin1);
      tank1Pins += ",trigPin2:";
      tank1Pins += String(trigPin2);
      tank1Pins += ",echoPin2:";
      tank1Pins += String(echoPin2);

      // LogSerial(true,logFunc,true,tank1Pins);
      break;
    case 2:
      trigPin1 = Tank2TrigPin1;
      echoPin1 = Tank2EchoPin1;
      trigPin2 = Tank2TrigPin2;
      echoPin2 = Tank2EchoPin2;
      break;
  }


  //repeat the same for both  sensors to average out the reading
  //Each tank has two sensror fo accuracy and backup
  float distanceSensor1 = 0;
  float distanceSensor2 = 0;
  float distance = 0;

  for (int sensor = 1; sensor <= SensorsMaxCount; sensor++)
  {
    int trigPin = 0;
    int echoPin = 0;
    long duration = 0;

    distance = 0;

    switch (sensor)
    {
      case 1:
        trigPin = trigPin1;
        echoPin = echoPin1;
        break;
      case 2:
        trigPin = trigPin2;
        echoPin = echoPin2;
        break;
    }

    //Do 2- 3 ticks to confirm the reading before considering as final reading
    //To avoid error readings

    float previousValue = 0;
    int errorValueCount = 0;

    for (int tickCount = 0; tickCount < MaxTickCount ; tickCount++)
    {
      float value = GetSensorValue(trigPin, echoPin);

      if (value < ErrorReading)
        previousValue += value;
      else
        errorValueCount++;

      delay(100);
    }//end of tick count loop

    //After all tick counts take average of distance
    distance = previousValue / (MaxTickCount - errorValueCount);

    if (sensor == 1)
      distanceSensor1 = distance;
    else if (sensor == 2)
      distanceSensor2 = distance;

    //delay(200);

  }//end of sensor loop

  //Average out distance from both sensors before sending
  if (distanceSensor1 > 0 && distanceSensor1 < ErrorReading)
  {
    if (distanceSensor2 > 0 && distanceSensor2 < ErrorReading)
    {
      distance = (distanceSensor1 + distanceSensor2) / SensorsMaxCount;
    }
    else
      distance = distanceSensor1;
  }
  else
  {
    if (distanceSensor2 > 0 && distanceSensor2 < ErrorReading)
      distance = distanceSensor2;
    else
      distance = -1;//error
  }

  // LogSerial(true,logFunc,true,String(distance));

  return distance;
}

float GetSensorValue(int trigPin, int echoPin)
{
  long duration = 0;
  float distance = 0;

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  //  LogSerial(false,logFunc,true,String(duration));

  // Calculating the distance
  distance = duration * 0.034 / 2;

  //distance in inches
  distance = distance * 0.393701;

  //If distance > 0 like 3274849 then make
  if (distance > ErrorReading)
  {
    LogSerial(true, logFunc, false, "Sensor error reading");
    digitalWrite(LED1, HIGH);
    return distance;
  }

  //  LogSerial(true,logFunc,true,String(distance));

  return distance;
}

//Log function
void LogSerial(bool nextLine, String function, bool flow, String msg)
{
  if (EnableDebug)
  {
    if (!flow)
      msg = function + " : " + msg;

    if (nextLine)
      Serial.println(msg);
    else
      Serial.print(msg);
  }
}
