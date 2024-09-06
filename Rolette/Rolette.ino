/*

    ************************************************************************************
    * MIT License
    *
    * Copyright (c) 2023 Crunchlabs LLC (IRTurret Control Code) 
    * Copyright (c) 2020-2022 Armin Joachimsmeyer (IRremote Library)
   
    * Permission is hereby granted, free of charge, to any person obtaining a copy
    * of this software and associated documentation files (the "Software"), to deal
    * in the Software without restriction, including without limitation the rights
    * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    * copies of the Software, and to permit persons to whom the Software is furnished
    * to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in all
    * copies or substantial portions of the Software.
    *
    * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
    * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
    * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
    * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    *
    ************************************************************************************
    */
   
   //////////////////////////////////////////////////
                  //  LIBRARIES  //
   //////////////////////////////////////////////////
   #include <Arduino.h>
   #include <Servo.h>
   #include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
   #include <IRremote.hpp>
   
   
   #define DECODE_NEC  //defines the type of IR transmission to decode based on the remote. See IRremote library for examples on how to decode other types of remote
   
   /*
   ** if you want to add other remotes (as long as they're on the same protocol above):
   ** press the desired button and look for a hex code similar to those below (ex: 0x11)
   ** then add a new line to #define newCmdName 0x11,
   ** and add a case to the switch statement like case newCmdName: 
   ** this will let you add new functions to buttons on other remotes!
   ** the best remotes to try are cheap LED remotes, some TV remotes, and some garage door openers
   */
   
   //defines the specific command code for each button on the remote
   #define left 0x8
   #define right 0x5A
   #define up 0x52
   #define down 0x18
   #define ok 0x1C
   #define cmd1 0x45
   #define cmd2 0x46
   #define cmd3 0x47
   #define cmd4 0x44
   #define cmd5 0x40
   #define cmd6 0x43
   #define cmd7 0x7
   #define cmd8 0x15
   #define cmd9 0x9
   #define cmd0 0x19
   #define star 0x16
   #define hashtag 0xD
   
   //////////////////////////////////////////////////
             //  PINS AND PARAMETERS  //
   //////////////////////////////////////////////////
   Servo yawServo; //names the servo responsible for YAW rotation, 360 spin around the base
   Servo pitchServo; //names the servo responsible for PITCH rotation, up and down tilt
   Servo rollServo; //names the servo responsible for ROLL rotation, spins the barrel to fire darts
   
   int yawServoVal; //initialize variables to store the current value of each servo
   int pitchServoVal = 100;
   int rollServoVal;
   
   int pitchMoveSpeed = 8; //this variable is the angle added to the pitch servo to control how quickly the PITCH servo moves - try values between 3 and 10
   int yawMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the YAW servo motor. It is added or subtracted from the yawStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Try values between 10 and 90;
   int yawStopSpeed = 90; //value to stop the yaw motor - keep this at 90
   int rollMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the ROLL servo motor. It is added or subtracted from the rollStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Keep this at 90 for best performance / highest torque from the roll motor when firing.
   int rollStopSpeed = 90; //value to stop the roll motor - keep this at 90
   
   int yawPrecision = 150; // this variable represents the time in milliseconds that the YAW motor will remain at it's set movement speed. Try values between 50 and 500 to start (500 milliseconds = 1/2 second)
   int rollPrecision = 165; // this variable represents the time in milliseconds that the ROLL motor with remain at it's set movement speed. If this ROLL motor is spinning more or less than 1/6th of a rotation when firing a single dart (one call of the fire(); command) you can try adjusting this value down or up slightly, but it should remain around the stock value (160ish) for best results.
   
   int pitchMax = 175; // this sets the maximum angle of the pitch servo to prevent it from crashing, it should remain below 180, and be greater than the pitchMin
   int pitchMin = 10; // this sets the minimum angle of the pitch servo to prevent it from crashing, it should remain above 0, and be less than the pitchMax
   
   int dartsFired = 0;
   
   //////////////////////////////////////////////////
                  //  S E T U P  //
   //////////////////////////////////////////////////
   void setup() {
       Serial.begin(9600);
   
       yawServo.attach(10); //attach YAW servo to pin 10
       pitchServo.attach(11); //attach PITCH servo to pin 11
       rollServo.attach(12); //attach ROLL servo to pin 12
       // Just to know which program is running on my microcontroller
       Serial.println(F("START " __FILE__ " from " __DATE__ "Using library version " VERSION_IRREMOTE));
       // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
       IrReceiver.begin(9, ENABLE_LED_FEEDBACK);
   
       Serial.print(F("Ready to receive IR signals of protocols: "));
       printActiveIRProtocols(&Serial);
       Serial.println(F("at pin " STR(9)));
   
       homeServos(); //set servo motors to home position
   }
   
   ////////////////////////////////////////////////
                  //  L O O P  //
   ////////////////////////////////////////////////
   
   void loop() {
   
       /*
        * Check if received data is available and if yes, try to decode it.
        */
       if (IrReceiver.decode()) {
   
           /*
            * Print a short summary of received data
            */
           IrReceiver.printIRResultShort(&Serial);
           IrReceiver.printIRSendUsage(&Serial);
           if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
               Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
           }
           Serial.println();
   
           /*
            * !!!Important!!! Enable receiving of the next value,
            * since receiving has stopped after the end of the current received data packet.
            */
           IrReceiver.resume(); // Enable receiving of the next value
   
           /*
            * Finally, check the received data and perform actions according to the received command
            */
   
           switch(IrReceiver.decodedIRData.command){ //this is where the commands are executed
   
               case up://pitch up
                 upMove(1);
                 break;
               
               case down://pitch down
                 downMove(1);
                 break;
   
               case left://fast counterclockwise rotation
                 leftMove(1);
                 break;
               
               case right://fast clockwise rotation
                 rightMove(1);
                 break;
               
               case ok: //firing routine 
                 fire(1);
                 break;
                 
               case star:
                 fireAll();
                 delay(50);
                 break;
   
               case hashtag:
                 randomRoulette();
               break;
   
               default:
               
               break;
   
           }
       }
       delay(5);
   }
   
   void shakeHeadYes(int moves = 2) {
         Serial.println("YES");
       int startAngle = pitchServoVal; // Current position of the pitch servo
       int lastAngle = pitchServoVal;
       int nodAngle = startAngle + 15; // Angle for nodding motion
   
       for (int i = 0; i < moves; i++) { // Repeat nodding motion two times
           // Nod up
           for (int angle = startAngle; angle <= nodAngle; angle++) {
               pitchServo.write(angle);
               delay(5); // Adjust delay for smoother motion
           }
           delay(50); // Pause at nodding position
           // Nod down
           for (int angle = nodAngle; angle >= startAngle; angle--) {
               pitchServo.write(angle);
               delay(5); // Adjust delay for smoother motion
           }
           delay(50); // Pause at starting position
       }
   }
   
   void shakeHeadNo(int moves = 2) {
       Serial.println("NO");
       int startAngle = pitchServoVal; // Current position of the pitch servo
       int lastAngle = pitchServoVal;
       int nodAngle = startAngle + 60; // Angle for nodding motion
   
       for (int i = 0; i < moves; i++) { // Repeat nodding motion two times
           // rotate right, stop, then rotate left, stop
           yawServo.write(150);
           delay(170); // Adjust delay for smoother motion
           yawServo.write(yawStopSpeed);
           delay(50);
           yawServo.write(30);
           delay(170); // Adjust delay for smoother motion
           yawServo.write(yawStopSpeed);
           delay(50); // Pause at starting position
       }
   }
   
   void leftMove(int moves){ // function to move left (for continuous servos only (YAW, ROLL))
       for (int i = 0; i < moves; i++){
           yawServo.write(yawStopSpeed + yawMoveSpeed); // adding the servo speed = 180 (full counterclockwise rotation speed)
           delay(yawPrecision); // stay rotating for a certain number of milliseconds
           yawServo.write(yawStopSpeed); // stop rotating
           delay(5); //delay for smoothness
           Serial.println("LEFT");
     }
   
   }
   
   void rightMove(int moves){ // function to move right (for continuous servos only (YAW, ROLL))
     for (int i = 0; i < moves; i++){
         yawServo.write(yawStopSpeed - yawMoveSpeed); //subtracting the servo speed = 0 (full clockwise rotation speed)
         delay(yawPrecision);
         yawServo.write(yawStopSpeed);
         delay(5);
         Serial.println("RIGHT");
     }
   }
   
   void upMove(int moves){ 
     for (int i = 0; i < moves; i++){
         if(pitchServoVal > pitchMin){//make sure the servo is within rotation limits (greater than 10 degrees by default)
           pitchServoVal = pitchServoVal - pitchMoveSpeed; //decrement the current angle and update
           pitchServo.write(pitchServoVal);
           delay(50);
           Serial.println("UP");
         }
     }
   }
   
   void downMove (int moves){ // function to handle down movement
     for (int i = 0; i < moves; i++){
           if(pitchServoVal < pitchMax){ // make sure the servo is within rotation limits (less than 175 degrees by default)
           pitchServoVal = pitchServoVal + pitchMoveSpeed;// increment the current angle and update
           pitchServo.write(pitchServoVal);
           delay(50);
           Serial.println("DOWN");
         }
     }
   }
   
   void fire(int darts){ //function for firing a single dart
     Serial.print("Firing ");
     Serial.print(darts);
     Serial.println(" darts!");
     for(int i = 0; i < darts; i++){
         rollServo.write(rollStopSpeed + rollMoveSpeed);//start rotating the servo
         delay(rollPrecision);//time for approximately 60 degrees of rotation
         rollServo.write(rollStopSpeed);//stop rotating the servo
         delay(5); //delay for smoothness
         if (dartsFired < 6){ // track how many darts have been fired
           dartsFired++;
         } else {
           dartsFired = 6; //lock the number at 6
           return;
         }
     }
   }
   
   void fireAll(){ // this function fires all of the darts by spinning the barrel
       rollServo.write(rollStopSpeed + rollMoveSpeed);//start rotating the servo
       delay(rollPrecision * 6); //time for 360 degrees of rotation
       rollServo.write(rollStopSpeed);//stop rotating the servo
       delay(5); // delay for smoothness
       Serial.println("FIRING ALL DARTS");
       dartsFired = 6;
     }
   
   
   /*
   ** this function acts as a form of Roulette, in which a spinning turret stops periodically
   ** rolls a (simulated) 6 sided die each time it stops in order to decide whether it should fire or not.
   */
   void randomRoulette() {
     Serial.println("ENTERING ROULETTE MODE");
     //unsigned long startTime = millis();
     dartsFired = 0;
     while(dartsFired < 6){ //while we still have darts, stay within this while loop
       if (dartsFired < 6){ // if we still have darts do stuff (this is redundancy to help break out of the while loop in case something weird happens)
         pitchServoVal = 110;
         pitchServo.write(pitchServoVal); // set PITCH servo to flat angle
         yawServo.write(145); // set YAW to rotate slowly
         delay(400); // rotating for a moment
         yawServo.write(90); // stop
         delay(400 * random(1,4)); //wait for a random delay each time
   
         if(random(3) == 0){ // you have a 1 in six chance of being hit
           delay(700); 
           if(random(2) == 0){ // 50% chance to either shake the head yes before firing or just fire
             shakeHeadYes();
             delay(150);
             fire(1); // fire 1 dart
             delay(100);
           // } else if(random(6) == 0){
           //   shakeHeadYes();
           //   delay(50);
           //   fireAll(); // fire all the darts
           //   delay(50);
           } else {
             fire(1); // fire 1 dart
             delay(50);
           }
         }else{
           if(random(6) == 5){
             delay(700);
             shakeHeadNo();
             delay(300);
           } else{
             delay(700);
           }
         }
       } else{
         yawServo.write(90); // redundancy to stop the spinning and break out of the while loop
         return;
       }
     }
     yawServo.write(90); // finally, stop the yaw movement
   }
   
   void homeServos(){
       yawServo.write(yawStopSpeed); //setup YAW servo to be STOPPED (90)
       delay(20);
       rollServo.write(rollStopSpeed); //setup ROLL servo to be STOPPED (90)
       delay(100);
       pitchServo.write(100); //set PITCH servo to 100 degree position
       delay(100);
       pitchServoVal = 100; // store the pitch servo value
       Serial.println("HOMING");
   }   