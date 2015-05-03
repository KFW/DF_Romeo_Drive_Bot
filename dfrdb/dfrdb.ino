/*
dfrdb
DFRobot Romeo Drive Bot
Robot for MakerFaire Kansas City Arch Reactor booth

Read RC signal pulse width by using interrupt then control servos of
SparkFun robot arm (https://www.sparkfun.com/products/11524 and  https://www.sparkfun.com/products/11674 )


DFRobot Romeo v2 controller board 
http://www.dfrobot.com/wiki/index.php/Romeo_V2-All_in_one_Controller_(R3)_(SKU:DFR0225)

Board has pin config like Leonardo - plan to take advantage of fact that Leonardo has 4 hardware
interrupts for my 4 controller signals; see http://www.arduino.cc/en/Reference/AttachInterrupt 

                   
Helpful resources:
http://gammon.com.au/interrupts
http://rcarduino.blogspot.com/2012/04/how-to-read-multiple-rc-channels-draft.html
http://rcarduino.blogspot.com/2012/01/how-to-read-rc-receiver-with.html
https://beyondszine.wordpress.com/2013/10/31/part-ii-all-pin-interrupt-method-for-rc-in-arduino/


*/


#include <Servo.h>

Servo wrist;
Servo claw;

const int WristInterrupt = 3; // interrupt 0 is on pin 3 for wrist (throttle on receiver)
const int ClawInterrupt = 2; 
// interrupt 1 is on pin 2 for claw (rudder on receiver)
const int WristPin = 12;
const int ClawPin = 13;

volatile unsigned long wristPW = 0;   // volatile since will be in interrupt function
volatile unsigned long clawPW = 0;
volatile unsigned long wristTimer = 0;
volatile unsigned long clawTimer = 0;
volatile boolean flag = false;
unsigned long wristPulse = 0;
unsigned long clawPulse = 0;

int clawAngle = 85;
int clawMinDeadspaceLimit = 1400; // threshold for closing claw; want dead zone in center
int clawMaxDeadspaceLimit = 1600; // threshold for opening claw
const int minClawAngle = 35;      // limit travel to protect servo
const int maxClawAngle = 135;

void setup() {
  attachInterrupt(0, readWristInterrupt, CHANGE);  // pin 3 on Romeo  __ signal for wrist
  attachInterrupt(1, readClawInterrupt, CHANGE);   // pin 2            __ signal for claw
  wrist.attach(WristPin);
  claw.attach(ClawPin);
  wrist.write(90); // starting position
  wrist.write(clawAngle);
} // end setup()

void loop() {
  // use flag to see if there is actually a new signal; may need to use more specific flags in future
  if (flag){
    noInterrupts(); // pause interrupts so pw signals are read faithfully and not changed part way during assignment
    wristPulse = wristPW;
    clawPulse = clawPW;
    flag = false;
    interrupts();   // there is a safer way to do this by saving and resorting settings register, but since I'm not using libraries should be OK
    
    wrist.writeMicroseconds(wristPulse);
    
    // for claw want to open/close base on stick, but then hold position; so increment/decrement depending on position
    // and then keep that value if joystick centered
    if (clawPulse < clawMinDeadspaceLimit){
      clawAngle -= 1; 
    }
    else if (clawPulse > clawMaxDeadspaceLimit){
      clawAngle +=1;
    }
    clawAngle = constrain(clawAngle, minClawAngle, maxClawAngle);
    claw.write(clawAngle);
  }
} // end loop()

void readWristInterrupt() {
  // interrupt occurs on CHANGE - need to determine if now high (start of pulse) or low (end of pulse
  if(digitalRead(WristInterrupt) == HIGH){
    wristTimer = micros();  // get time stamp for initiation of pulse
  }
  else{   
    // pin must now be low - end of pulse; calculate pulse width
    wristPW = micros() - wristTimer;
    flag = true;
  }
} // end readWristInterrupt()

void readClawInterrupt() {
  // interrupt occurs on CHANGE - need to determine if now high (start of pulse) or low (end of pulse
  if(digitalRead(ClawInterrupt) == HIGH){
    clawTimer = micros();  // get time stamp for initiation of pulse
  }
  else{   
    // pin must now be low - end of pulse; calculate pulse width
    clawPW = micros() - clawTimer;
    flag = true;
  }
} // end readClawInterrupt()
