/*****************************************************
 * Rover 3 *A.S.T.R.A.
 * PICO CODE
 * v0.3.6
 * 10/15/23
 *****************************************************/

/* Notes:
 ********
 * I'm not sure that the OpenLog works but it doesn't throw errors so...
*/

/* Libraries used:
 *****************
 * Adafruit BMP3XX Library (not 107)
 * Adafruit BNO055         (not sparkfun IMU)
 * Adafruit Unified Sensor
 * idk where Servo SPI or Wire comes from...
 * SoftwareSerial is included by default
 */
//Headers
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Adafruit_BNO055.h>
#include <Servo.h>
#include <SoftwareSerial.h>



//Pins
//#define SD_RESET_PIN    0 //DTR/CTS??? DTR on schematic
#define SD_TX_PIN       0 //UART TX to OpenLog RX
//#define SD_RX_PIN       2 //UART RX to OpenLog TX
#define I2C_SDA_PIN     4 //I2C SDA
#define I2C_SCL_PIN     5 //I2C SCL
#define SERVO_1_PIN    18 //Metal Gear Servos
#define SERVO_2_PIN    19 // "
#define SERVO_3_PIN    20 // "
#define SERVO_4_PIN    21 //High Torque Servo
#define MOTOR_IN2_PIN  14 //Named "MOTOR_AIN2_IO" on schematic
#define MOTOR_PWM_PIN  15 //Motor Controller PWM
#define MOTOR_STBY_PIN 16 //Motor stby pin
#define MOTOR_IN1_PIN  17 //Named "MOTOR_AIN1_IO" on schematic



//Predeclarations
#define SEALEVELPRESSURE_HPA (1013.25)
const int SERVO_MIN =  500; //milliseconds
const int SERVO_MAX = 2500; //milliseconds
const int SERIAL_BAUD = 115200;
// OPENLOG
double currTime = millis();
double prevOpenLogTime = currTime;
double openLogInterval = 500; //Write data to the openLog twice per second


//Setup hardware vars
Adafruit_BMP3XX bmp;
//                                    id,addr
Adafruit_BNO055 bno = Adafruit_BNO055(55,0x28,&Wire);
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;



//****************************************************
void setup() {

  //Initialization
  //**************
  // Initialize LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
  // Turn LED on for initialization
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Configure serial transport
  Wire.begin(); //Join I2C bus
  Serial.begin(SERIAL_BAUD); //115200 set in original code, 9600 default for pico
  delay(100);

  //OpenLog
  Serial1.begin(9600);
  delay(1000);
  
  // Turn LED off after serial initialization
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Initialization complete.");


  //Servos
  //******
  servo1.attach(SERVO_1_PIN, SERVO_MIN, SERVO_MAX);
  servo2.attach(SERVO_2_PIN, SERVO_MIN, SERVO_MAX);
  servo3.attach(SERVO_3_PIN, SERVO_MIN, SERVO_MAX);
  servo4.attach(SERVO_4_PIN, SERVO_MIN, SERVO_MAX);

  //Motor
  //*****
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_STBY_PIN, OUTPUT);


  //BMP Sensor
  //**********
  // Setup bmp I2C
  if (!bmp.begin_I2C()) {   // hardware I2C mode
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    //while (1);
  }
  /*int i = 0;
  while(i < 100 && !bmp.begin_I2C()) {
    i++;
    delay(50);
  }*/
  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);


  //BNO Sensor
  //**********
  if (!bno.begin()) {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }
  //Not much to do here lol


  //OpenLog (SD)
  //************
  //Uses SoftwareSerial w/ TX pin
  Serial1.print("Time, Temperature, Pressure, AccelX, AccelY, AccelZ\n");

  Serial.println("Hardware configured.");
  Serial.println(millis());
  Serial.println("-------------");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
}



//****************************************************
void loop() {
  /*Command interpreter
   ********************
   * To see a list of commands, see 'help' command
   */
  if (Serial.available()) {
    //Take in str input from Serial
    String input = Serial.readStringUntil('\n');
    input.trim();
    //input = toLowerCase(input);

    //Parse into array separated by spaces
    String args[20];
    int argCount = 0;
    while (input.length() > 0) {
      //Stolen from https://forum.arduino.cc/t/how-to-split-a-string-with-space-and-store-the-items-in-array/888813
      int index = input.indexOf(' ');
      if (index == -1) { // No space found
        args[argCount++] = input;
        break;
      } else {
        args[argCount++] = input.substring(0, index);
        input = input.substring(index+1);
      }
    }
    String command = args[0];

    /*
     * command is args[0] or just command
     * First argument is args[1]
     * Second is args[2], etc
     */

    //Commands
    //****************
    if        (command == "led_on") {
      digitalWrite(LED_BUILTIN, HIGH);

    } else if (command == "led_off") {
      digitalWrite(LED_BUILTIN, LOW);

    } else if (command == "ping") {
      Serial.print(millis());
      Serial.println("-pong");

    } else if (command == "time") {
      Serial.println(millis());

    } else if (command == "line") {
      Serial.println("-------------");
    
    } else if (command == "help") {
      //Serial.println("Sounds like it sucks to be you");
      Serial.println("led_on; led_off; ping; time; line; pollbmp; pollbno; setservo;");
      Serial.println("setmotor; testwritesd");

    //Sensors
    //*******
    
    } else if (command == "pollbmp") {
      if (! bmp.performReading()) {
        Serial.println("Failed to perform reading :(");
        return;
      }
      //temp record
      Serial.print("Temp: ");
      Serial.print(bmp.temperature);
      Serial.println(" *C");
      //presure record
      Serial.print("Pressure: ");
      Serial.print(bmp.pressure);
      Serial.println(" Pa");
      //approx altitude
      //Serial.print("Approx. Altitude = ");
      //Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
      //Serial.println(" m");
    
    } else if (command == "beginbmp") {
      bmp.begin_I2C();
    
    } else if (command == "pollbno") {
      if (! bmp.performReading()) {
        Serial.println("Failed to perform reading :(");
        return;
      }
      sensors_event_t linearAccelData, accelerometerData;
      bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
      bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
      Serial.print("Accel: ");
      Serial.println(getBNOData(&accelerometerData));
    
    //Servos
    //******

    } else if(command == "setservo") {
      int servoNum = args[1].toInt(); //First arg is servo number (ex servo1, servo2, etc)
      int pos = args[2].toInt(); //Second arg is num to set the servo to
      bool err = false; //Set true if the servo specified is disable in preprocessor statements

      if(servoNum != 0) { //If the servo number provided is a real number
        //Set servo depending on which servo
        //Error if the servo is not enabled in preprocessor statements
        switch(servoNum) {
          case 1:
            servo1.write(pos);
            break;
          case 2:
            servo2.write(pos);
            break;
          case 3:
            servo3.write(pos);
            break;
          case 4:
            servo4.write(pos);
            break;
          default:
            //Did not select one of 4 servos, though real number given
            Serial.println("Error: select a servo with a number 1-4");
            err = true;
            break;
        }
        //Print a success message if no error occured
        if(!err) {
          Serial.print("Set servo ");
          Serial.print(servoNum);
          Serial.print(" to ");
          Serial.print(pos);
          Serial.println(".");
        }
      } else {
        Serial.println("Error: invalid servo number");
      }
    
    } else if (command == "setallservos") {
      int servoPow = args[1].toInt();
      if(servoPow != 0) {
        servo1.write(servoPow);
        servo2.write(servoPow);
        servo3.write(servoPow);
        servo4.write(servoPow);
      } else {
        Serial.println("Error: invalid value for servo power");
      }
    
    } else if (command == "sr") {
      int servo1Pow = args[1].toInt();
      int servo2Pow = args[2].toInt();
      int servo3Pow = args[3].toInt();
      int servo4Pow = args[4].toInt();
      if(servo1Pow != 0 && servo2Pow != 0 && servo3Pow != 0 && servo4Pow != 0) {
        servo1.write(servo1Pow);
        servo2.write(servo2Pow);
        servo3.write(servo3Pow);
        servo4.write(servo4Pow);
      } else {
        Serial.println("Error: invalid value for one or more servo's power");
      }

    //Motor
    //*****
    
    } else if (command == "setmotor") {
      int motorSpeed = args[1].toInt();
      int motorDir = args[2].toInt(); // 1 or 2
      if(motorSpeed != 0 && motorDir != 0) {
        move(motorSpeed, motorDir-1);
        Serial.println("Motor ran.");
      } else {
        Serial.println("Error: Invalid motor speed or direction provided");
      }
    
    } else if (command == "stopmotor") {
      stop();
      Serial.println("Motor stopped.");
    

    //OpenLog
    //*******
    
    } else if(command == "testwritesd") {
      Serial1.print("This is a test write of the OpenLog.\n");
    }
  }


  //Collect BNO Data
  //****************
  currTime = millis();
  if(prevOpenLogTime + openLogInterval <= currTime) {
    //BMP & BNO
    if (! bmp.performReading()) {
      Serial.println("Failed to perform reading :(");
      return;
    }
    sensors_event_t linearAccelData, accelerometerData;
    bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
    bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);

    Serial1.print(currTime);
    Serial1.print(", ");
    Serial1.print(bmp.temperature);
    Serial1.print(", ");
    Serial1.print(bmp.pressure);
    Serial1.print(", ");
    Serial1.print(getBNOData(&accelerometerData));
    Serial1.print("\n");
    prevOpenLogTime += openLogInterval;
  }

}

//BNO
String getBNOData(sensors_event_t* event) {
  String output = "";
  output.concat(event->acceleration.x);
  output.concat(", ");
  output.concat(event->acceleration.y);
  output.concat(", ");
  output.concat(event->acceleration.z);
  return output;
}
double getBNOX(sensors_event_t* event) {
  return event->acceleration.x;
}
double getBNOY(sensors_event_t* event) {
  return event->acceleration.y;
}
double getBNOZ(sensors_event_t* event) {
  return event->acceleration.z;
}

//Motor functions

void move(int speed, int direction){
  //Stolen from https://adam-meyer.com/arduino/TB6612FNG
  //Move motor at speed and direction
  //speed: 0 is off, and 255 is full speed
  //direction: 0 clockwise, 1 counter-clockwise

  digitalWrite(MOTOR_STBY_PIN, HIGH); //disable standby

  boolean inPin1 = LOW;
  boolean inPin2 = HIGH;

  if(direction == 1){
    inPin1 = HIGH;
    inPin2 = LOW;
  }

  digitalWrite(MOTOR_IN1_PIN, inPin1);
  digitalWrite(MOTOR_IN2_PIN, inPin2);
  analogWrite(MOTOR_PWM_PIN, speed);
}

void stop(){
  //enable standby  
  digitalWrite(MOTOR_STBY_PIN, LOW);
}
