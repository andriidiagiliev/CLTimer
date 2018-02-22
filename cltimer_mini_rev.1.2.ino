// CL Timer - Timer for Indoor Control Line models with BLC/BC controllers
// Rev. 1.2
// (c) Andrii Diagilev - dyagilev@gmail.com

#include <Servo.h>
#include <EEPROM.h>    

// Timing settings
#define START_DELAY  3000 // ms
#define START_TIME   3000 // ms
#define STOP_TIME    3000 // ms

// Hardware connections
#define BUTTON_PIN   10 // 14 PB2 - 10 - OC1B - PCINT2 - PWM - SS
#define POT_PIN      A0 // 23 PC0 - 14 A0 - PCINT8 - ADC0
#define LED_PIN       7 // 11 PD7 - 7 - AIN1 - PCINT23
#define BLC_PIN       2 // 32 PD2 - 2 - INT0 - PCINT18

#define WORKTIME_ADDR 0 // EEPROM address for Working Time value

Servo blc;

int operation_mode = 0; // Stand-by mode by default
int pot_value = 0; // analog value from potentiometer
boolean led_is_on = false; // False - OFF, True- ON
boolean is_button_pressed = false; // False - not pressed, True - pressed
int pulse_width = 0; // blc pulse width (0...180 degrees) set to 0;
int working_time;    // working time in sec.

// ------------------------------------------------------
// Read the Button state with Debounce 

boolean debounce(int polls, int concurrences, int reqdelay) { 

// polls - number of button polls
// concurrences - number of required concurrences for debounce
// reqdelay - delay between the polls, delay = polls x reqdelay  
// return: true - if the Button has been pressed
  
  int i, j, concount;
  
  concount = 0; // value of calculated concurrences
  
  for(i = 0; i < polls; i++){
    if (!digitalRead(BUTTON_PIN)) concount++;
     for (j = 0; j < 5; j++){
      blc.writeMicroseconds(1050);  // защита от срабатывания failsafe на регуляторе по отсутствию сигнала от таймера            
      delay(reqdelay);
     };
    
  };

  if(concurrences < concount){
    return true;
  } else{
     return false;
    };
}


// ------------------------------------------------------
// Get pulse width from the potentiometer 
// pot_pin - ADC pin connected to the potentiometer
// return: value for the servo position depending on the potentiometer position (1050..1950 mks)

int get_ppm_mks(int pot_pin){
 
 int value;
 
  value = analogRead(pot_pin);   
  if(value < 255) value = map(value, 0, 254, 1050, 1499); // exp - section 0...25%%      
  if(value > 254 && value < 511) value = map(value, 255, 510, 1500, 1724); // exp - section 26...50%%
  if(value > 510 && value < 1024) value = map(value, 511, 1023, 1725, 1950); // exp - section 51...100%%     
  value = map(value, 0, 1023, 1050, 1950); //   

 return value;
}


// ------------------------------------------------------
// Initial settings
void setup() {
  
  pinMode(BLC_PIN, OUTPUT);
  digitalWrite(BLC_PIN, LOW);  
  blc.attach(BLC_PIN); 
  blc.writeMicroseconds(1050);

  pinMode(POT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH); // connect the Button pin to the internal resistor
  pinMode(LED_PIN, OUTPUT); // LED is connected to VCC with 330 Om resistor
  digitalWrite(LED_PIN, HIGH); // LED is OFF by deffault
    
  EEPROM.get(WORKTIME_ADDR, working_time); // get the Working time from EEPROM

  is_button_pressed = debounce(100, 70, 10); // 100 requests, 70 concurrences, 10 ms delay

      if (is_button_pressed){
        operation_mode = 1; // go to Programming mode
        is_button_pressed = false;        
      };
}

// ------------------------------------------------------
void loop() {

  unsigned int  i, j, current_pulse, wrk_minutes, wrk_10seconds; 
  int delta;

  switch (operation_mode) {
    
    case 0: // Stand-by mode

      wrk_minutes = working_time / 60;
      wrk_10seconds = (working_time - (wrk_minutes *60)) / 10;

      for(i = 0; i < wrk_minutes; i++){
        digitalWrite(LED_PIN, LOW); // LED is ON
        for (j = 0; j < 40; j++){
          blc.writeMicroseconds(1050);
          delay(10);
        };
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        for (j = 0; j < 60; j++){
          blc.writeMicroseconds(1050);            
          delay(10);
        };
       };

      is_button_pressed = debounce(60, 40, 10); // 60 requests, 40 concurrences, 10 ms delay = 600 ms
      
      if (is_button_pressed){
        operation_mode = 2; // go to Start mode
        is_button_pressed = false; 
        break;       
      };
            
      for(i = 0; i < wrk_10seconds; i++){
        digitalWrite(LED_PIN, LOW); // LED is ON
        for (j = 0; j < 20; j++){
          blc.writeMicroseconds(1050);
          delay(10);
        };
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        for (j = 0; j < 30; j++){
          blc.writeMicroseconds(1050);            
          delay(10);
        };
       };
      
      is_button_pressed = debounce(60, 40, 10); // 60 requests, 40 concurrences, 10 ms delay = 600 ms
      
      if (is_button_pressed){
        operation_mode = 2; // go to Start mode
        is_button_pressed = false;        
      };
      break;
    
    case 1: // Programming mode
          
      for (i = 0; i < 10; i++){      // LED: On - 150 ms, Off - 150 ms - indication that Programming mode has started
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        delay(150);
        digitalWrite(LED_PIN, LOW); // LED is ON
        delay(150);        
      };
      
      while(true){

        wrk_minutes = working_time / 60;
        wrk_10seconds = (working_time - (wrk_minutes *60)) / 10;
        
        for(i = 0; i < wrk_minutes; i++){
          digitalWrite(LED_PIN, LOW); // LED is ON
          for (j = 0; j < 40; j++){
            blc.writeMicroseconds(1050);
            delay(10);
          };
          digitalWrite(LED_PIN, HIGH); // LED is OFF
          for (j = 0; j < 60; j++){
            blc.writeMicroseconds(1050);            
            delay(10);
          };
        };
       
       delay (300);
       is_button_pressed = debounce(30, 20, 10); // 30 requests, 20 concurrences, 10 ms delay = 300 ms
       delay (300);
      
       if (is_button_pressed){
          wrk_minutes += 1; // increase working time for 1 minute
          if(wrk_minutes > 10){
            wrk_minutes = 0;
            if(wrk_10seconds = 0) wrk_10seconds = 1;
          };
          working_time = (wrk_minutes * 60) + (wrk_10seconds * 10);
          is_button_pressed = false;
          EEPROM.put(WORKTIME_ADDR, working_time);  // write Working Time value to EEPROM          
       };
            
      for(i = 0; i < wrk_10seconds; i++){
        digitalWrite(LED_PIN, LOW); // LED is ON
        for (j = 0; j < 20; j++){
          blc.writeMicroseconds(1050);
          delay(10);
        };
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        for (j = 0; j < 30; j++){
          blc.writeMicroseconds(1050);            
          delay(10);
        };
      };

      delay (300);
      is_button_pressed = debounce(30, 30, 10); // 30 requests, 20 concurrences, 10 ms delay = 300 ms
      delay (300);
      
      if (is_button_pressed){
          wrk_10seconds += 1; // increase working time for 10 sec
          if(wrk_10seconds > 5){
            wrk_10seconds = 0;
            wrk_minutes += 1;
            if(wrk_minutes > 10){
              wrk_minutes = 0;
              wrk_10seconds = 1;
            };
          };
          working_time = (wrk_minutes * 60) + (wrk_10seconds * 10);
          is_button_pressed = false;
          EEPROM.put(WORKTIME_ADDR, working_time);  // write Working Time value to EEPROM          
      };     

    };
      break;
    
    case 2: // Ready to start
      
      for (i = 0; i < (START_DELAY / 300); i++){ // LED: On - 150 ms, Off - 150 ms
       digitalWrite(LED_PIN, HIGH); // LED is OFF
       delay(155);
       digitalWrite(LED_PIN, LOW); // LED is ON
       delay(155);        
      };
      // pulse_width = get_ppm(POT_PIN); 
      pulse_width = get_ppm_mks(POT_PIN); 
      operation_mode = 3; // go to Start mode 
      break;

    case 3: // Start mode
    
        delta = pulse_width - 1050;
        for(i = 0; i < 30; i++){
          current_pulse = 1050 + (delta * i) / 30;
          blc.writeMicroseconds(current_pulse);
          if(i % 3){
            digitalWrite(LED_PIN, LOW); // LED is ON 
          } else {
            digitalWrite(LED_PIN, HIGH); // LED is OFF 
          };
            
        delay(START_TIME / 30);
      };

      operation_mode = 4; // go to Working mode
      break;

    case 4: // working mode

      digitalWrite(LED_PIN, LOW);               // LED is ON
      
     for(i = 0; i < (working_time * 20); i++){
        pulse_width = get_ppm_mks(POT_PIN);          // you can tune throttle at this moment
        blc.writeMicroseconds(pulse_width);   
        delay(25); //
        blc.writeMicroseconds(pulse_width);
        delay(25); // 
      };
       
      operation_mode = 5; // go to Stop mode
      break;

    case 5: // Stop mode

        delta = pulse_width - 1050;
        for(i = 0; i < 30; i++){
          current_pulse = pulse_width - (delta * i) / 30;
          blc.writeMicroseconds(current_pulse);
          if(i % 3){
            digitalWrite(LED_PIN, HIGH); // LED is ON 
          } else {
            digitalWrite(LED_PIN, LOW); // LED is OFF 
          };
            
        delay(START_TIME / 30);
      };

      operation_mode = 0; // go to Stand-By mode
      break;
  }
  
}
