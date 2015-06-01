/******************************************************************************
dfrdb
DFRobot Romeo Drive Bot
Robot for MakerFaire Kansas City Arch Reactor booth
drives just the claw - user Sabertooth for motor shield

Pin connections:
AIL:  (hold pin 0 for this use)
ELE:  (hold pin 1 for this use)
THR: 3
RUD: 2
power receiver off A5 since digital pins at 7.2V for servos

Read RC signal pulse width by using interrupt then control servos of
SparkFun robot arm (https://www.sparkfun.com/products/11524 and 
https://www.sparkfun.com/products/11674 )


DFRobot Romeo v2 controller board 
http://www.dfrobot.com/wiki/index.php/Romeo_V2-All_in_one_Controller_(R3)_(SKU:DFR0225)

Board has pin config like Leonardo - plan to take advantage of fact that 
Leonardo has 4 hardware interrupts for my 4 controller signals; 
see http://www.arduino.cc/en/Reference/AttachInterrupt 

                   
Helpful resources:
http://gammon.com.au/interrupts
http://rcarduino.blogspot.com/2012/04/how-to-read-multiple-rc-channels-draft.html
http://rcarduino.blogspot.com/2012/01/how-to-read-rc-receiver-with.html
https://beyondszine.wordpress.com/2013/10/31/part-ii-all-pin-interrupt-method-for-rc-in-arduino/

Using HobbyKing Turnigy 5X 5 channel mini transmitter and receiver
https://www.hobbyking.com/hobbyking/store/uh_viewItem.asp?idProduct=43854
Right joystick for steering/speed, Left for claw (fwd/back for arm position, 
side to side to open/close claw)

Will assume full range of signal (1000 - 2000), simplifies programming
Extreme values outside range only occur with extremes of trim, which 
should be a rare outlier - will avoid the overhead of a constrain() operation


******************************************************************************/

/********************
*** DECLARATIONS ****
********************/
#include <Servo.h>

Servo wrist;
Servo claw;

const int WristInterruptPin = 3; // interrupt 0 is on pin 3 for wrist
const int WristInterrupt = 0;
const int ClawInterruptPin = 2;
const int ClawInterrupt = 1;
const int WristPin = 12;
const int ClawPin = 13;

volatile unsigned long wristPW = 0;   // volatile since will be in interrupt 
                                      // function
volatile unsigned long clawPW = 0;
volatile unsigned long wristTimer = 0;
volatile unsigned long clawTimer = 0;
volatile boolean wristFlag = false;
volatile boolean clawFlag = false;
unsigned long wristPulse = 0;
unsigned long clawPulse = 0;

int clawAngle = 85;
int ClawRDZ = 1450; // threshold for closing claw - right limit of dead zone
int ClawLDZ = 1550; // threshold for opening claw
const int minClawAngle = 5;      // limit travel to protect servo
const int maxClawAngle = 135;

/********************
====== BODY =========
********************/

void setup() {
  attachInterrupt(WristInterrupt, readWristInterruptPin, CHANGE); 
  attachInterrupt(ClawInterrupt, readClawInterruptPin, CHANGE); 
  wrist.attach(WristPin);
  claw.attach(ClawPin);
  wrist.write(90); // starting position
  wrist.write(clawAngle);
} // end setup()

void loop() {
  // use flag to see if there is actually a new signal
  if (wristFlag){
    noInterrupts(); // pause interrupts so pw signals are read faithfully 
                    // and not changed part way during assignment
    wristPulse = wristPW;
    wristFlag = false;
    interrupts();   // there is a safer way to do this by saving and restoring 
                    // settings register, but since I'm not using extensive 
                    // libraries should be OK
    
    wrist.writeMicroseconds(wristPulse);
  }
  if (clawFlag){ 
    noInterrupts(); 
    clawPulse = clawPW;
    clawFlag = false;
    interrupts(); 
    // for claw want to open/close base on stick, but then hold position; 
    // so increment/decrement depending on position
    // and then keep that value if joystick centered
    if (clawPulse < ClawRDZ){
      clawAngle -= 1; 
    }
    else if (clawPulse > ClawLDZ){
      clawAngle +=1;
    }
    clawAngle = constrain(clawAngle, minClawAngle, maxClawAngle);
    claw.write(clawAngle);
  }
} // end loop()

/********************
----- FUNCTIONS -----
********************/

void readWristInterruptPin() {
  // interrupt occurs on CHANGE - need to determine if now high 
  // (start of pulse) or low (end of pulse
  if(digitalRead(WristInterruptPin) == HIGH){
    wristTimer = micros();  // get time stamp for initiation of pulse
  }
  else{   
    // pin must now be low - end of pulse; calculate pulse width
    wristPW = micros() - wristTimer;
    wristFlag = true;
  }
} // end readWristInterruptPin()

void readClawInterruptPin()        // works just like wrist interrupt
  if(digitalRead(ClawInterruptPin) == HIGH){
    clawTimer = micros(); 
  }
  else{
    clawPW = micros() - clawTimer;
    clawFlag = true;
  }
} // end readClawInterruptPin()
