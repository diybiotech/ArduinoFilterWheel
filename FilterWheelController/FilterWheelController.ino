

unsigned int version_ = 1;


void setup() {
    // Higher speeds do not appear to be reliable
    Serial.begin(57600);
}

void loop() {
  if (Serial.available() > 0) {
     int inByte = Serial.read();

     switch (inByte) {
        // Gives identification of the device
       case 1:
         Serial.println("Arduino-FW");
         break;

       // Returns version string
       case 2:
         Serial.println(version_);
         break;

       // Set position
       case 3:
          if (waitForSerial(timeOut_)) {

            // get position
            currentPattern_ = Serial.read();

            // todo: set filter wheel position



          }
          break;

     }
  }

}

 
bool waitForSerial(unsigned long timeOut)
{
    unsigned long startTime = millis();

    while (Serial.available() == 0 && (millis() - startTime < timeOut) ) {}

    return Serial.available() > 0;
}


