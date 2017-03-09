// Filter Wheel Controller
// Version 1.0
// BioCurious Fluoroscent Microscope
// By Shirish Goyal <shirish.goyal@gmail.com>
// Program loops waiting for serial events of new commands for different filter wheel positions.

#include <AFMotor.h>              // Invoke library for controlling the motor shield.
String input;                // a string to hold incoming data from the serial port
boolean hasNewInput = false;      // whether the incomming string is complete

AF_DCMotor motor(4);             // Select motor 4

int lastOpto, currentOpto, delta;
int lastHall, currentHall, deltaHall;
float position = -10;
int NONE = -100;
int monitor = NONE;
int direction = 1;
int MAX = 6;
int SPEED = 200;

int toDigital(int val){
  if (val > 512) {
    return 1;
  }
  else {
    return 0;
  }
}

int getOpto() {
  int val = analogRead(A0);
  return toDigital(val);
}

int getHall() {
  int val = analogRead(A1);
  //Serial.println(val);
  return toDigital(val);
}

void backward() {
  motor.run(BACKWARD);
  direction = -1;
  //Serial.println(" backwards");
}

void forward() {
  motor.run(FORWARD);
  direction = 1;
  //Serial.println(" forwards");
}

void stop() {
  motor.run(RELEASE);
  direction = 1;
  //Serial.println("stop");
  monitor = -100;
}

void rotate(int currPos, int pos) {
  int distance = currPos < pos ? (6 - pos + currPos) : (currPos - pos);
  //Serial.print(currPos);
  //Serial.print("=>");
  //Serial.print(pos);

  if (distance < 3) {
    backward();
  } else {
    forward();
  }
}

bool isPosition(int pos) {
  return (position == pos);
}

bool isHallFired() {
  return currentHall == 0 && lastHall == 1;
}

bool isOptoFired() {
  return currentOpto == 0 && lastOpto == 1;
}

bool shouldStop() {
  return monitor != NONE && isPosition(monitor);
}

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps

  // reserve 200 bytes for the input:
  input.reserve(200);
  //Serial.println("Initializing...");

  // turn on motor
  motor.setSpeed(SPEED);
  motor.run(RELEASE);

  lastOpto = getOpto();
  lastHall = getHall();

  monitor = 6;
  forward();
}

//Main Loop
void loop() {
  if (shouldStop()) {
    stop();
  }

  currentOpto = getOpto();
  currentHall = getHall();

  if (isHallFired())
  {
    position = 5.5;
    //Serial.println(position);
  }

  if (isOptoFired())
  {
    if (position == 5.5) {
      position = position + (direction * 0.5);
    } else {
      position = position + direction;
    }

    if (position == 7) {
      position = 1;
    }

    if (position == 0) {
      position = 6;
    }
    
    //Serial.println(position);
  }

  lastOpto = currentOpto;
  lastHall = currentHall;

  // print the string when a newline arrives:
  if (hasNewInput) {
    if (input[0] == 'V') {
      Serial.println("ArduinoFilterWheel");
    }else if(input[0] == '0') {
      stop();
    }else if (input[0] == '1'
        || input[0] == '2'
        || input[0] == '3'
        || input[0] == '4'
        || input[0] == '5'
        || input[0] == '6') {
      int pos = input.substring(0).toInt();

      if (!isPosition(pos)) {
        rotate(position, pos);
        monitor = pos;
      }
    }else {
      Serial.print("Invalid command: ");
      Serial.println(input);
    }

    //Clear the input string:
    input = "";
    hasNewInput = false;
  }
}

/*
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\r') {
      hasNewInput = true;
    }
    else {
      // add it to the input:
      input += inChar;
    }
  }
}

