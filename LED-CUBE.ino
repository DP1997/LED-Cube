#include <IRremoteInt.h>
#include <ir_Lego_PF_BitStreamEncoder.h>
#include <IRremote.h>
#include <boarddefs.h>

//initializing and declaring led layers
int layer[4]={A0,A1,A2,A3};
//initializing and declaring led rows (except the ones that are controlled by the shift register)
int column[11]={A5,A4,5,6,7,8,9,10,11,12,13}; 

//pin connected to SH_CP of 74HC595
int clockPin = 0;
//pin connected to ST_CP of 74HC595
int latchPin = 1;
//pin connected to DS of 74HC595
int dataPin = 2;
//pin connected to the infrared receiver
int RECV_PIN = 3;
//pin connected to the sound module
int soundPin = 4;

//declare digital pin 3 as the ir receiver
IRrecv irrecv(RECV_PIN);
//make the reiceived data globally known
decode_results results;

//all states the cube can be in
enum State{
  S_SOUND, S_SPIRAL, S_PROPELLER, S_SNAKE, S_FIREWORK, S_LAYERBOUNCE, S_WAVE, S_RRAIN, S_RFLICKER, S_PULSING, S_AUTO, S_OFF, S_ON
};

//state of the LED cube; default is turnEverythingOn()
volatile State state = S_ON;

//sound values: used to play different patterns dependent on the value of v
int v = 0;
int w = 0; 

//the infrared remote control (sender) sends hexdecimal values to the receiver
//in order to be able to distinguish between them we have mapped them to their respective function 
#define OFF           0x00FFA25D          // CH     
#define ON            0x00FFC23D          // >||
#define SOUND         0x00FF906F          //EQ
#define AUTO          0x00FF6897          // 0      
#define SPIRAL        0x00FF30CF          // 1         
#define PROPELLER     0x00FF18E7          // 2         
#define SNAKE         0x00FF7A85          // 3        
#define FIREWORK      0x00FF10EF          // 4         
#define LAYERBOUNCE   0x00FF38C7          // 5          
#define WAVE          0x00FF5AA5          // 6        
#define RRAIN         0x00FF42BD          // 7        
#define RFLICKER      0x00FF4AB5          // 8   
#define PULSING       0x00FF52AD          // 9
 
void setup(){

  //set each column of the cube to output
  for(int i =0 ; i<11; i++){
    pinMode(column[i], OUTPUT);
  }
  //set each layer of the cube to output
  for(int i = 0; i<4; i++){
    pinMode(layer[i], OUTPUT);  
  }
  //set pins to output in order to be able to address and control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(soundPin, INPUT);

  //listen for interrupts on digital pin 3 (INT1)
  attachInterrupt(1, isr, RISING);
  //start the receiver
  irrecv.enableIRIn(); 
}

//play a pattern corresponding to the current state
void loop(){

  switch(state){
    
    case S_SOUND: {   //read whether or not the microphone is detecting sound (GROUND)and act accordingly
                      boolean sound = digitalRead(soundPin);
                      //sound detected
                      if (sound==0) {
                        //display pattern
                        soundcube();
                        //makes the LEDs brighter
                        delay(1);
                      }
                      //no sound detected
                      else turnEverythingOff();
                   }
                      break;
                  
    case S_SPIRAL:  spiral();
                    break;
                  
    case S_PROPELLER: propeller();
                      break;

    case S_SNAKE: snake();
                  break;

    case S_FIREWORK: firework();
                   break;

    case S_LAYERBOUNCE: horizontalLayerBounce();
                        verticalLayerBounce();
                        break;

    case S_WAVE: wave();
                 break;

    case S_RRAIN: randomRain();
                  break;

    case S_RFLICKER: randomFlicker();
                     break;

    case S_PULSING: pulsingCube();
                    break;

    case S_AUTO: 
                //play a predefined set of patterns
                for(int i=0; i<3; i++){
                  if (state != S_AUTO) break; spiral();
                }
                 for(int i=0; i<20; i++) {
                  if (state != S_AUTO) break; propeller(); 
                 }
                 for(int i=0; i<3; i++) {
                  if (state != S_AUTO) break; snake(); 
                 }
                 for(int i=0; i<15; i++){
                  if (state != S_AUTO) break; firework();
                 }
                 for(int i=0; i<7; i++) {
                  if (state != S_AUTO) break; horizontalLayerBounce(); verticalLayerBounce(); 
                 }
                 for(int i=0; i<20; i++){
                  if (state != S_AUTO) break; wave();
                 }
                 for(int i=0; i<60; i++){
                  if (state != S_AUTO) break; randomRain(); 
                 }
                 for(int i=0; i<1000; i++){
                  if (state != S_AUTO) break; randomFlicker();
                 }
                 for(int i=0; i<15; i++) {
                  if (state != S_AUTO) break; pulsingCube(); 
                 }
                 break;
    
    case S_ON:  turnEverythingOn();
                break;
                
    case S_OFF: turnEverythingOff();
                break;
    default: break;
  }

}

//the interrupt service routine is responsible for determining the next state depending on the button bressed on the remote control
void isr(){
  
  while (irrecv.decode(&results)){
     if     (results.value == SOUND) state = S_SOUND;             //button "EQ" pressed
     else if(results.value == SPIRAL) state = S_SPIRAL;           //1
     else if(results.value == PROPELLER) state = S_PROPELLER;     //2
     else if(results.value == SNAKE) state = S_SNAKE;             //3
     else if(results.value == FIREWORK) state = S_FIREWORK;       //4
     else if(results.value == LAYERBOUNCE) state = S_LAYERBOUNCE; //5
     else if(results.value == WAVE) state = S_WAVE;               //6
     else if(results.value == RRAIN) state = S_RRAIN;             //7
     else if(results.value == RFLICKER) state = S_RFLICKER;       //8
     else if(results.value == PULSING) state = S_PULSING;         //9
     else if(results.value == AUTO) state = S_AUTO;               //0
     else if(results.value == OFF) state = S_OFF;                 //CH-
     else if(results.value == ON) state = S_ON;                   //>||
     
     //ready to receive the next value
     irrecv.resume(); 

    }
}

//digital pins 1-5 cannot be accessed directly since they are connected to the shift register
//to access them, we will have to address them seperately through said register
void setShiftRegister(unsigned char uc){
  
  //make the latchPin low so that the LEDs don't change while sending in bits
  digitalWrite(latchPin, LOW);
  //shift out the bits
  shiftOut(dataPin, clockPin, MSBFIRST, uc);  
  //make the latchPin high so the send byte is displayed by the outputs of the register
  digitalWrite(latchPin, HIGH);
}

//display the pattern that is shown when sound is detected
void soundcube(){
  
   //play two different patterns according to the value of v & w
    v++;
    w++;
    if(v<100){
      digitalWrite(layer[1], 1);
      digitalWrite(layer[2], 1);
      setShiftRegister(231);
      digitalWrite(column[4], 0);
      digitalWrite(column[5], 0);  
    }else if(v >= 100 && v <= 300){ 
      digitalWrite(layer[0], 1);
      digitalWrite(layer[1], 1);
      digitalWrite(layer[2], 1);
      digitalWrite(layer[3], 1);    
      digitalWrite(column[0], 0);
      digitalWrite(column[7], 0);
      digitalWrite(column[10], 0);      
      setShiftRegister(253);    
     } 
      else if(v>300){
      if(v > 400)v = 0;
      if(w % 1200 == 0) {
        propeller();
        w = 0;
      }
      digitalWrite(layer[0], 1);
      digitalWrite(layer[1], 1);
      digitalWrite(layer[2], 1);
      digitalWrite(layer[3], 1);
      digitalWrite(column[1], 0);
      setShiftRegister(250);
      digitalWrite(column[2], 0);
      digitalWrite(column[3], 0);
      digitalWrite(column[6], 0);
      digitalWrite(column[8], 0);
      digitalWrite(column[9], 0);
      }
}
    
 
//turn all LEDs off (inverting the adjacent power)
void turnEverythingOff(){

  //setting the layerPins(A0-A3) to 0
  for(int i = 0; i<4; i++){
    digitalWrite(layer[i], 0);
  }
  //setting all colums that are directly controlled by the arduino to 1
  for(int i = 0; i<11; i++){
    digitalWrite(column[i], 1);
  }
  //turn off LED columns controlled by the shift register
  setShiftRegister(255);
}
 
//turn all LEDs on
void turnEverythingOn(){

  //turn all layers on by applying 5V
  for(int i = 0; i<4; i++){
    digitalWrite(layer[i], 1);
  }
  //turn all columns on which are controlled by the arduino by applying ground/0V
  for(int i = 0; i<11; i++){
    digitalWrite(column[i], 0);
  }
  //turn all columns on (apply ground) which are controlled by the shift register
  setShiftRegister(0);
}

//turn all columns off by applying 5V to each one
void turnAllColumnsOff(){

  //turn all columns controlled by the arduino off
  for(int i = 0; i<11; i++){
    digitalWrite(column[i], 1);
  }
  //turn all columns controlled by the shift register off
  setShiftRegister(255);
}

//turn all columns on by applying ground to each one
void turnAllColumnsOn(){

  //turn all columns controlled by the arduino on
  for(int i=0; i<11; i++){
    digitalWrite(column[i], 0);
  }
  //turn all columns controlled by the shift register on
  setShiftRegister(0);
}

void turnAllLayersOn(){
  for(int i=0; i<4; i++){
    digitalWrite(layer[i], 1);
  }
}

//turn all LEDs on and off in decreasing intervals
void flicker(){
  int i = 150;
  while(i > 0){
    turnEverythingOn();
    delay(i);
    turnEverythingOff();
    delay(i);
    i-= 5;
  }
}

//add layers from bottom to top (turn them on) until peaked (4 layers are on) then substract them again (turn them off layer by layer)
void horizontalLayerBounce(){

  //defined state from which we can work from
  turnEverythingOff();
  turnAllColumnsOn();
  //time before next layer gets added or subtracted
  int delayTime = 50;
  //add layers from bottom to top 
  for(int i=0; i<4; i++){
    digitalWrite(layer[i], 1);
    delay(delayTime);
  }
  //substract layers from bottom to top
  for(int i=0; i<4; i++){
    digitalWrite(layer[i], 0);
    delay(delayTime);
  }
  //add layers from top to bottom
  for(int i=3; i>=0; i--){
    digitalWrite(layer[i], 1);
    delay(delayTime);
  }
  //substract layers from top to bottom
  for(int i=3; i>=0; i--){
    digitalWrite(layer[i], 0);
    delay(delayTime);
  }
}

void verticalLayerBounce(){
  
  //defined state from which we can work from
  turnEverythingOff();
  turnAllLayersOn();
  //time before next layer gets added or subtracted
  int delayTime = 50;

  //light up first layer
  digitalWrite(column[0], 0);
  digitalWrite(column[1], 0);
  setShiftRegister(252);
  delay(delayTime);
  //light up second layer
  setShiftRegister(0);
  digitalWrite(column[2], 0);
  delay(delayTime);
  //light up third layer
  for(int i=3; i<7; i++){
    digitalWrite(column[i], 0);
  }
  delay(delayTime);
  //light up fourth layer
  for(int i=7; i<11; i++){
    digitalWrite(column[i], 0);
  }
  delay(delayTime);

  //darken first layer
  digitalWrite(column[0], 1);
  digitalWrite(column[1], 1);
  setShiftRegister(3);
  delay(delayTime);
  //darken second layer
  setShiftRegister(255);
  digitalWrite(column[2], 1);
  delay(delayTime);
  //darken third layer
  for(int i=3; i<7; i++){
    digitalWrite(column[i], 1);
  }
  delay(delayTime);
  //darken fourth layer
  for(int i=7; i<11; i++){
    digitalWrite(column[i], 1);
  }
  delay(delayTime);

  //light up fourth layer
  for(int i=7; i<11; i++){
    digitalWrite(column[i], 0);
  }
  delay(delayTime);
  //light up third layer
  for(int i=3; i<7; i++){
    digitalWrite(column[i], 0);
  }
  delay(delayTime);
  //light up second layer
  setShiftRegister(227);
  digitalWrite(column[2], 0);
  delay(delayTime);
  //light up first layer
  digitalWrite(column[0], 0);
  digitalWrite(column[1], 0);
  setShiftRegister(0);
  delay(delayTime);

  //darken fourth layer
  for(int i=7; i<11; i++){
    digitalWrite(column[i], 1);
  }
  delay(delayTime);
  //darken third layer
  for(int i=3; i<7; i++){
    digitalWrite(column[i], 1);
  }
  delay(delayTime);
  //darken second layer
  setShiftRegister(252);
  digitalWrite(column[2], 1);
  delay(delayTime);
  //darken first layer
  digitalWrite(column[0], 1);
  digitalWrite(column[1], 1);
  setShiftRegister(255);
  delay(delayTime);
}

//a snake that is four LEDs long is slithering its way through the cube!
void snake(){
  
  turnEverythingOff();
  int delayTime = 120;
  // the snake is 4 LEDs long
  
  // layer 1
  snakeLayerA(delayTime, 0);
  // layer 2 
  snakeLayerB(delayTime, 1); 
  // layer 3
  snakeLayerA(delayTime, 2);
  // layer 4
  snakeLayerB(delayTime, 3);    
}

void snakeLayerA(int delayTime, int l){
  
    if (l == 2) digitalWrite(layer[1], 0);
    
    digitalWrite(layer[l], 1);
    for(int i = 0; i < 4; i++){
      digitalWrite(column[10-i], 0);
      delay(delayTime);
    }
    for(int i= 0; i < 4; i++){
      digitalWrite(column[3+i], 0);
      digitalWrite(column[10-i],1);
      delay(delayTime);
    }
    digitalWrite(column[2], 0);
    digitalWrite(column[3], 1);
    delay(delayTime);
    setShiftRegister(239);
    digitalWrite(column[4], 1);
    delay(delayTime);
    setShiftRegister(231);
    digitalWrite(column[5], 1);
    delay(delayTime);
    setShiftRegister(227);
    digitalWrite(column[6], 1);
    delay(delayTime);
    digitalWrite(column[2],1);
    digitalWrite(column[0], 0);
    delay(delayTime);
    setShiftRegister(243);
    digitalWrite(column[1], 0);
    delay(delayTime);
    setShiftRegister(250);
    delay(delayTime);
    setShiftRegister(252);
    delay(delayTime);
}
  
void snakeLayerB(int delayTime, int l){
  digitalWrite(column[0], 1);
  delay(delayTime / 2);
  digitalWrite(column[1], 1);
  delay(delayTime / 2);
  setShiftRegister(253);
  delay(delayTime / 2);
  digitalWrite(layer[l], 1);    
  // snake is now on layer 2
  digitalWrite(layer[l-1], 0);
  delay(delayTime);
  setShiftRegister(252);
  delay(delayTime);
  digitalWrite(column[1], 0);
  delay(delayTime);
  digitalWrite(column[0], 0);
  delay(delayTime);
  setShiftRegister(250);
  delay(delayTime);
  setShiftRegister(243);
  delay(delayTime);
  digitalWrite(column[1], 1);
  setShiftRegister(227);
  delay(delayTime);
  digitalWrite(column[0], 1);
  digitalWrite(column[2], 0);
  delay(delayTime);
  digitalWrite(column[6], 0);
  setShiftRegister(231);
  delay(delayTime);
  digitalWrite(column[5], 0);
  setShiftRegister(239);
  delay(delayTime);
  digitalWrite(column[4], 0);
  setShiftRegister(255);    
  delay(delayTime);
  digitalWrite(column[2], 1);
  digitalWrite(column[3], 0);
  delay(delayTime);
  for(int i = 0; i < 4; i++){
    digitalWrite(column[7 + i], 0);
    digitalWrite(column[6 - i], 1);
    delay(delayTime);
  }
  for(int i = 0; i < 3; i++){
    digitalWrite(column[7 + i], 1);
    delay(delayTime / 2);
  }
  if (l == 3){
    // back to beginning
    digitalWrite(layer[3], 1);
    delay(delayTime / 2);
    for(int i = 0; i < 3; i++){
      digitalWrite(layer[2 - i], 1);
      digitalWrite(column[10], 0);
      delay(delayTime / 2);
    }
    for(int i = 0; i < 4; i++){
      digitalWrite(layer[3 - i], 0);
    }
  }
}

//shoots up a LED in a single column, exploding at the top 
void firework(){
  
    turnEverythingOff();
    int delayTime = 150;
    
    digitalWrite(layer[0], 1);

    int randomColumn = random(0,16);
    if(randomColumn == 11){
        for(int i = 0; i < 3; i++){
        setShiftRegister(255);
        delay(delayTime/2);
        setShiftRegister(254);
        delay(delayTime/2);
        }
    }
    else if(randomColumn == 12){
        for(int i = 0; i < 3; i++){
        setShiftRegister(255);
        delay(delayTime/2);
        setShiftRegister(253);
        delay(delayTime/2);
        }
    }
    else if(randomColumn == 13){
        for(int i = 0; i < 3; i++){
        setShiftRegister(255);
        delay(delayTime/2);
        setShiftRegister(251);
        delay(delayTime/2);
        }
    }
    else if(randomColumn == 14){
        for(int i = 0; i < 3; i++){
        setShiftRegister(255);
        delay(delayTime/2);
        setShiftRegister(247);
        delay(delayTime/2);
        }
    }
    else if(randomColumn == 15){
        for(int i = 0; i < 3; i++){
        setShiftRegister(255);
        delay(delayTime/2);
        setShiftRegister(239);
        delay(delayTime/2);
        }
    }
    else{
          for(int i = 0; i < 4; i++){
        digitalWrite(column[randomColumn], 1);
        delay(delayTime / 2);
        digitalWrite(column[randomColumn], 0);
        delay(delayTime / 2);
        }      
    }
    for(int i = 1; i < 4; i++){
    digitalWrite(layer[i], 1);
    delay(delayTime/2);
      }
    for(int i = 0; i < 3; i++){
    digitalWrite(layer[i], 0);
    delay(delayTime/2);
      }
    turnAllColumnsOn();
    delay(delayTime); // firework exploding
    turnEverythingOff();
    fireworkSparkling();
}

//creates the fading sparkling effect after the explosion at the top
void fireworkSparkling(){
  
  int delayTime = 10;
  digitalWrite(layer[3], 1);
  for(int i = 0; i < 70; i++){
  int randomColumn = random(0,16);
  if(randomColumn == 11){
    setShiftRegister(254);
  }
  else if(randomColumn == 12){
    setShiftRegister(253);
  }
  else if(randomColumn == 13){
    setShiftRegister(251);
  }
  else if(randomColumn == 14){
    setShiftRegister(247);
  }
  else if(randomColumn == 15){
    setShiftRegister(239);
  }
  else{
      digitalWrite(column[randomColumn], 0);
  }
  delay(delayTime);
  if(randomColumn<11) digitalWrite(column[randomColumn], 1);
  else setShiftRegister(255);
  if (i == 40) delayTime += 15; // this is for slowing down the sparkling effect at the end of the rocketexplosion
  }
}

//2x2 cube which extends until all LEDs are turned on
void pulsingCube(){

  //defined, secured state to start from
  turnEverythingOff();
  int delayTime = 75;
  
  //2x2 cube
  digitalWrite(layer[1], 1);
  digitalWrite(layer[2], 1);
  setShiftRegister(231);
  digitalWrite(column[4], 0);
  digitalWrite(column[5], 0);
  delay(delayTime);
  turnEverythingOff();
  delay(delayTime);

  //extended 2x2 cube
  digitalWrite(layer[1], 1);
  digitalWrite(layer[2], 1);
  setShiftRegister(2);
  digitalWrite(column[4], 0);
  digitalWrite(column[5], 0);
  digitalWrite(layer[0], 1);
  digitalWrite(layer[3], 1);
  digitalWrite(column[1], 0);
  digitalWrite(column[2], 0);
  digitalWrite(column[3], 0);
  digitalWrite(column[6], 0);
  digitalWrite(column[8], 0);
  digitalWrite(column[9], 0);
  delay(delayTime);
  turnEverythingOff();
  delay(delayTime);

  //4x4 cube
  turnEverythingOn();
  delay(delayTime);
  turnEverythingOff();
  delay(delayTime);

  //extended 2x2 cube
  digitalWrite(layer[1], 1);
  digitalWrite(layer[2], 1);
  setShiftRegister(2);
  digitalWrite(column[4], 0);
  digitalWrite(column[5], 0);
  digitalWrite(layer[0], 1);
  digitalWrite(layer[3], 1);
  digitalWrite(column[1], 0);
  digitalWrite(column[2], 0);
  digitalWrite(column[3], 0);
  digitalWrite(column[6], 0);
  digitalWrite(column[8], 0);
  digitalWrite(column[9], 0);
  delay(delayTime);
  turnEverythingOff();
  delay(delayTime);

  //2x2 cube
  digitalWrite(layer[1], 1);
  digitalWrite(layer[2], 1);
  setShiftRegister(231);
  digitalWrite(column[4], 0);
  digitalWrite(column[5], 0);
  delay(delayTime);
  turnEverythingOff();
  delay(delayTime);
}

//creates a spiral that is working itself clockwise from the inner part of the cube to the outer and backwise 
//and then starts over counterclockwise
void spiral(){
  //defined, secured state to start from
  turnEverythingOff();
  turnAllLayersOn();
  int delayTime = 75;

  //clockwise
  setShiftRegister(247);
  delay(delayTime);
  setShiftRegister(231);
  delay(delayTime);
  digitalWrite(column[5], 0);
  delay(delayTime);
  digitalWrite(column[4], 0);
  delay(delayTime);
  digitalWrite(column[3], 0);
  delay(delayTime);
  setShiftRegister(227);
  delay(delayTime);
  digitalWrite(column[0], 0);
  delay(delayTime);
  digitalWrite(column[1], 0);
  delay(delayTime);
  setShiftRegister(226);
  delay(delayTime);
  setShiftRegister(224);
  delay(delayTime);
  digitalWrite(column[2], 0);
  delay(delayTime);
  digitalWrite(column[6], 0);
  delay(delayTime);
  digitalWrite(column[10], 0);
  delay(delayTime);
  digitalWrite(column[9], 0);
  delay(delayTime);
  digitalWrite(column[8], 0);       
  delay(delayTime);
  digitalWrite(column[7], 0);
  delay(delayTime);

  //resolve the spiral clockwise
  digitalWrite(column[7], 1);
  delay(delayTime);
  digitalWrite(column[3], 1);
  delay(delayTime);
  setShiftRegister(228);
  delay(delayTime);
  digitalWrite(column[0], 1);
  delay(delayTime);
  digitalWrite(column[1], 1);
  delay(delayTime);
  setShiftRegister(229);
  delay(delayTime);
  setShiftRegister(231);
  delay(delayTime);
  digitalWrite(column[2], 1);
  delay(delayTime);
  digitalWrite(column[6], 1);
  delay(delayTime);
  digitalWrite(column[10], 1);
  delay(delayTime);
  digitalWrite(column[9], 1);
  delay(delayTime);
  digitalWrite(column[8], 1);
  delay(delayTime);
  digitalWrite(column[4], 1);
  delay(delayTime);
  digitalWrite(column[3], 1);
  delay(delayTime);
  setShiftRegister(239);
  delay(delayTime);
  setShiftRegister(255);
  delay(delayTime);
  digitalWrite(column[5], 1);
  delay(delayTime);

  //spiral counterclockwise
  digitalWrite(column[5], 0);
  delay(delayTime);
  setShiftRegister(239);
  delay(delayTime);
  setShiftRegister(231);
  delay(delayTime);
  digitalWrite(column[4], 0);
  delay(delayTime);
  digitalWrite(column[8], 0);
  delay(delayTime);
  digitalWrite(column[9], 0);
  delay(delayTime);
  digitalWrite(column[10], 0);
  delay(delayTime);
  digitalWrite(column[6], 0);
  delay(delayTime);
  digitalWrite(column[2], 0);
  delay(delayTime);
  setShiftRegister(229);
  delay(delayTime);
  setShiftRegister(228);
  delay(delayTime);
  digitalWrite(column[1], 0);
  delay(delayTime);
  digitalWrite(column[0], 0);
  delay(delayTime);
  setShiftRegister(224);
  delay(delayTime);
  digitalWrite(column[3], 0);
  delay(delayTime);
  digitalWrite(column[7], 0);
  delay(delayTime);

  //resolve the spiral counterclockwise
  digitalWrite(column[7], 1);
  delay(delayTime);
  digitalWrite(column[8], 1);
  delay(delayTime);
  digitalWrite(column[9], 1);
  delay(delayTime);
  digitalWrite(column[10], 1);
  delay(delayTime);
  digitalWrite(column[6], 1);
  delay(delayTime);
  digitalWrite(column[2], 1);
  delay(delayTime);
  setShiftRegister(226);
  delay(delayTime);
  setShiftRegister(227);
  delay(delayTime);
  digitalWrite(column[1], 1);
  delay(delayTime);
  digitalWrite(column[0], 1);
  delay(delayTime);
  setShiftRegister(231);
  delay(delayTime);
  digitalWrite(column[3], 1);
  delay(delayTime);
  digitalWrite(column[4], 1);
  delay(delayTime);
  digitalWrite(column[5], 1);
  delay(delayTime);
  digitalWrite(column[3], 1);
  delay(delayTime);
  setShiftRegister(247);
  delay(delayTime);
  setShiftRegister(255);
  delay(delayTime);
}

//turns a propeller counterclockwise all layers in height
void propeller(){

  //working base
  turnEverythingOff();
  turnAllLayersOn();
  int delayTime = 50;

  //diagonal layer
  digitalWrite(column[7], 0);
  digitalWrite(column[4], 0);
  setShiftRegister(237);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
  //next turn
  setShiftRegister(238);
  //digitalWrite(column[7], 1);
  digitalWrite(column[4], 0);
  digitalWrite(column[8], 0);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
  //next turn
  setShiftRegister(247);
//  digitalWrite(column[4], 1);
//  digitalWrite(column[8], 1);
  digitalWrite(column[5], 0);
  digitalWrite(column[1], 0);
  digitalWrite(column[9], 0);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
  //next turn
//  digitalWrite(column[1], 1);
//  digitalWrite(column[9], 1);
  setShiftRegister(247);
  digitalWrite(column[5], 0);
  digitalWrite(column[0], 0);
  digitalWrite(column[10], 0);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
  //next turn
  setShiftRegister(243);
 // digitalWrite(column[10], 1);
 // digitalWrite(column[0], 1);
  digitalWrite(column[5], 0);
  digitalWrite(column[6], 0);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
  //next turn
//  digitalWrite(column[6], 1);
//  digitalWrite(column[5], 1);
  digitalWrite(column[2], 0);
  digitalWrite(column[3], 0);
  digitalWrite(column[4], 0);
  delay(delayTime);
  //turn cube off for pulsing effect
  turnEverythingOff();
  turnAllLayersOn();
  delay(delayTime);
 
}

//creates raindrops, falling down from the heighest layer - spawned at random
void randomRain(){

    turnEverythingOff();
    int delayTime = 75;
    int randomColumn = random(0,16);
    if(randomColumn == 11){
      setShiftRegister(254);
    }
    else if(randomColumn == 12){
      setShiftRegister(253);
    }
    else if(randomColumn == 13){
      setShiftRegister(251);
    }
    else if(randomColumn == 14){
      setShiftRegister(247);
    }
    else if(randomColumn == 15){
      setShiftRegister(239);
    }
    else{
        digitalWrite(column[randomColumn], 0);
    }
    int direction = random(0,2);
    if(direction==0) {
      digitalWrite(layer[3], 1);
      delay(delayTime);
      //digitalWrite(layer[3], 0);
      digitalWrite(layer[2], 1);
      delay(delayTime);
      digitalWrite(layer[2], 0);
      digitalWrite(layer[1], 1);
      delay(delayTime);
      digitalWrite(layer[1], 0);
      digitalWrite(layer[0], 1);
      delay(delayTime);
      digitalWrite(layer[0], 0);
      delay(delayTime);
    } else{
      digitalWrite(layer[0], 1);
      delay(delayTime);
      digitalWrite(layer[1], 1);
      delay(delayTime);
      digitalWrite(layer[1], 0);
      digitalWrite(layer[2], 1);
      delay(delayTime);
      digitalWrite(layer[2], 0);
      digitalWrite(layer[3], 1);
      delay(delayTime);
      digitalWrite(layer[3], 0);
    }
    if(randomColumn < 11) digitalWrite(column[randomColumn], 1);
    else setShiftRegister(255);
}

//light up any LED of the cube at random
void randomFlicker(){

  //defined state to work with
  turnEverythingOff();
  int delayTime = 10;

  //choose a column to apply ground to at random
  int randomColumn = random(0, 16);
  //since the columns controlled by the shift register cannot be accessed through the array
  //we will have to check for these manually
  if(randomColumn == 11){
    setShiftRegister(254);
  }
  else if(randomColumn == 12){
    setShiftRegister(253);
  }
  else if(randomColumn == 13){
    setShiftRegister(251);
  }
  else if(randomColumn == 14){
    setShiftRegister(247);
  }
  else if(randomColumn == 15){
    setShiftRegister(239);
  }
  else{
    digitalWrite(column[randomColumn], 0);
  }
  //choose a layer to apply 5V at random
  int randomLayer = random(0, 4);
  digitalWrite(layer[randomLayer], 1);
  delay(delayTime);
  //turn the random LED off
  if(randomColumn>10) setShiftRegister(255);
  else digitalWrite(column[randomColumn], 1);
  digitalWrite(layer[randomLayer], 0);
  delay(delayTime);
}

//creates a wave motion, which is working its way through the cube
void wave(){

  turnEverythingOff();
  int delayTime=75;
  
  digitalWrite(layer[0], 1);
  digitalWrite(layer[1], 1);
  digitalWrite(layer[2], 1);
  digitalWrite(layer[3], 1);
  
  digitalWrite(column[0], 0);
  setShiftRegister(247);
  digitalWrite(column[5], 0);
  digitalWrite(column[10], 0);
  delay(delayTime);
  
  digitalWrite(column[1], 0);
  setShiftRegister(231);
  digitalWrite(column[6], 0);
  digitalWrite(column[7], 0);
  delay(delayTime);
  
  digitalWrite(column[0], 1);
  setShiftRegister(239);
  digitalWrite(column[5], 1);
  digitalWrite(column[10], 1);
  delay(delayTime);
  
  setShiftRegister(238);
  digitalWrite(column[2], 0);
  digitalWrite(column[3], 0);
  digitalWrite(column[8], 0);
  delay(delayTime);
  
  digitalWrite(column[1], 1);
  setShiftRegister(254);
  digitalWrite(column[6], 1);
  digitalWrite(column[7], 1);
  delay(delayTime);
  
  setShiftRegister(252);
  setShiftRegister(248);
  digitalWrite(column[4], 0);
  digitalWrite(column[9], 0);
  delay(delayTime);
  
  setShiftRegister(249);
  digitalWrite(column[2], 1);
  digitalWrite(column[3], 1);
  digitalWrite(column[8], 1);
  delay(delayTime);
  
  setShiftRegister(243);
  setShiftRegister(247);
  digitalWrite(column[4], 1);
  digitalWrite(column[9], 1);
  
}
