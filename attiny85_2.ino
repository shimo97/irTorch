/* ATtiny85 IR Remote Control Receiver

   David Johnson-Davies - www.technoblogy.com - 3rd April 2015
   ATtiny85 @ 1 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

//***********Edited bi Shimo for GSProjects***************
//https://www.youtube.com/channel/UCntR6zd7agg69MXz9oa-Yqw/featured

//ir receiver, button and light variables
short int lightPin=3;
short int buttPin=4;

int lightstate = 0; //1 on 0 off

int buttstate=LOW;
int buttstate_ex=LOW;
unsigned long debouncing_time_ms=50;
int value_save=LOW;
bool confirmed=false;

unsigned long second_pression_time_ms=1000;
short pression_number=0;
int pression_old=0;
unsigned long time_save;


volatile int NextBit;
volatile unsigned long RecdData;
int Brightness;
volatile boolean ir_ok=false;
unsigned long storedValue;
byte address,command;

// Interrupt service routine - called on every falling edge of PB2
ISR(INT0_vect) {
  int Time = TCNT0;
  int Overflow = TIFR & 1<<TOV0;
  // Keep looking for AGC pulse and gap
  if (NextBit == 32) {
    if ((Time >= 194) && (Time <= 228) && (Overflow == 0)) {
      RecdData = 0; NextBit = 0;
    }
  // Data bit
  } else {
    if ((Time > 44) || (Overflow != 0)) NextBit = 32; // Invalid - restart
    else {
      if (Time > 26) RecdData = RecdData | ((unsigned long) 1<<NextBit);
      if (NextBit == 31){
        GIMSK = GIMSK | 0<<INT0;    // Disable INT0
        storedValue=RecdData;
        ir_ok=true;
        } 
      NextBit++;
    }
  }
  TCNT0 = 0;                  // Clear counter
  TIFR = TIFR | 1<<TOV0;      // Clear overflow
  GIFR = GIFR | 1<<INTF0;     // Clear INT0 flag
}

// Setup **********************************************
  
void setup() {
  // Set up Timer/Counter0 (assumes 1MHz clock)
  TCCR0A = 0;                 // No compare matches
  TCCR0B = 3<<CS00;           // Prescaler /64
  // Set up INT0 interrupt on PB2
  MCUCR = MCUCR | 2<<ISC00;   // Interrupt on falling edge
  GIMSK = GIMSK | 1<<INT0;    // Enable INT0
  NextBit = 32;               // Wait for AGC start pulse

  //setup button and light pins
  pinMode(lightPin,OUTPUT);
  pinMode(buttPin,INPUT_PULLUP);
  
  digitalWrite(lightPin,LOW);
}

void toggleLight(){
 
  if(lightstate==HIGH) lightstate=LOW;
  else lightstate=HIGH;
}

void loop() {
  //Polling button state
  buttstate_ex=buttstate;
  buttstate=digitalRead(buttPin);
  if(buttstate!=buttstate_ex){
    value_save=buttstate;
    delay(debouncing_time_ms);
    buttstate=digitalRead(buttPin);
    if(buttstate==value_save){  //button change not confirmed
      confirmed=true;
    }
    if(confirmed==true){
        //toggle light on press
        if(buttstate_ex==HIGH && buttstate==LOW){
            toggleLight();
            digitalWrite(lightPin,lightstate);
        }
      confirmed=false;
    }
  }

  //ir value ready
  if(ir_ok){                                    // If the mcu receives NEC message with successful
    
    ir_ok = false;                               // Reset decoding process
    command = RecdData >> 16;
/*
    digitalWrite(lightPin,HIGH);
    delay(100);
    digitalWrite(lightPin,LOW);
    delay(100);
    digitalWrite(lightPin,HIGH);
    delay(100);
    digitalWrite(lightPin,LOW);
    delay(100);
    
    for(int c=0;c<8;c++){
      digitalWrite(lightPin,HIGH);
      delay(200);
      digitalWrite(lightPin,bitRead(command, c));
      delay(500);
      digitalWrite(lightPin,LOW);
      delay(200);
    }
    digitalWrite(lightPin,HIGH);
    delay(100);
    digitalWrite(lightPin,LOW);
    delay(100);
    digitalWrite(lightPin,HIGH);
    delay(100);
    digitalWrite(lightPin,LOW);
    delay(100);
 */     
    
    //second pression logic
    bool confirmed=false;
    if(command==3 || command==2){
      if( pression_number==0){
        pression_old=command;
        time_save=millis();
        pression_number=1;    
      }else{
        if(pression_old==command){
          unsigned long act_time=millis();
          if(act_time>time_save && act_time-time_save<second_pression_time_ms){ //ACT_TIME VALUE CONTROL + MILLIS OVERFLOW CONTROL
            confirmed=true;
            pression_number=0;
          }else{
            pression_old=command;
            time_save=millis();
            pression_number=1;  
          }
        }else{
            pression_old=command;
            time_save=millis();
            pression_number=1;  
        }
      }
      
    }

    //changing light state
    if(confirmed==true){
      if(command==3){
        lightstate=HIGH;
      }
      if(command==2){
        lightstate=LOW;
      }  
    }
    digitalWrite(lightPin,lightstate);
    GIMSK = GIMSK | 1<<INT0;    // Enable INT0
  }

}
