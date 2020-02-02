// CL Timer - Timer for Indoor Control Line models with BLC/BC controllers
//
// Designed to control the model's motor, configured and controlled by one button, the indication is made by one LED
// programmable time period: from 10 seconds up to 8 minutes 50 seconds
//
// Rev. 1.3
// (c) Andrii Diagilev - dyagilev@gmail.com

#include <Servo.h>
#include <EEPROM.h>

// Hardware connections
#define BUTTON_PIN   3   // Button
#define POT_PIN      A2  // Potentiometer
#define LED_PIN      12  // Main LED
#define BLC_PIN      4   // output to BLC
#define INT_LED_PIN  13  // internal LED on Board

// in-line
#define LED_is_ON()   ( digitalWrite(LED_PIN, LOW) )
#define LED_is_OFF()  ( digitalWrite(LED_PIN, HIGH) )
#define int_LED_is_ON()   ( digitalWrite(INT_LED_PIN, HIGH) )
#define int_LED_is_OFF()  ( digitalWrite(INT_LED_PIN, LOW) )

// Timing settings
#define START_DELAY  3000 // ms
#define START_TIME   3000 // ms
#define STOP_TIME    3000 // ms

// EEPROM Map
#define SIGNATURE_ADDR  0  // EEPROM address for Signature (char)
#define WORKTIME_ADDR   10 // EEPROM address for Working Time value (int)
#define MIN_TROT_ADDR   12 // EEPROM address for minimal value from poitentiometer for Trottle (int)
#define MAX_TROT_ADDR   14 // EEPROM address for maximal value from poitentiometer for Trottle (int)

#define DEBUG_LEVEL     2  // 

Servo blc;

int operation_mode = 0;             // Stand-by mode by default
int pot_value = 0;                  // analog value from the potentiometer
int pot_min_value;                  // min value from the potentiometer
int pot_max_value;                  // max value from the potentiometer
boolean led_is_on = false;          // False - OFF, True- ON
boolean is_button_pressed = false;  // False - not pressed, True - pressed
int pulse_width = 0;                // blc pulse width (0...180 degrees) set to 0;
int working_time;                   // working time in sec.
int sign = 0;

// ------------------------------------------------------
// Read the Button state with Debounce

boolean debounce(int polls, int concurrences, int reqdelay) {

  // polls - number of button polls
  // concurrences - number of required concurrences for debounce
  // reqdelay - delay (ms) between the polls
  // return: true - if the Button has been pressed

  int i, j, concount;

  concount = 0; // value of calculated concurrences

  for (i = 0; i < polls; i++) {
    if (!digitalRead(BUTTON_PIN)) concount++;

    blc.writeMicroseconds(1050);  // to avoid failsafe on BLC
    delay(reqdelay);
  };

  if (concurrences < concount) {
    return true;
  } else {
    return false;
  };
}

// ------------------------------------------------------
// Get pulse width from the potentiometer
// pot_pin - ADC pin connected to the potentiometer
// return: value for the servo position depending on the potentiometer position (1050..1950 mks)

int get_ppm_mks(int pot_pin) {

  int value;

  value = analogRead(pot_pin);

#if DEBUG_LEVEL > 2
      Serial.print("Pot_analog = ");
      Serial.println(value, DEC);      
#endif
  
  if (value < pot_min_value) value = pot_min_value;
  if (value > pot_max_value) value = pot_max_value;

#if DEBUG_LEVEL > 2
      Serial.print("pot_min_value = ");
      Serial.println(pot_min_value, DEC);      
      Serial.print("pot_max_value = ");
      Serial.println(pot_max_value, DEC);
      Serial.print("Pot >< = ");
      Serial.println(value, DEC);      
#endif

  value = map(value, pot_min_value, pot_max_value, 0, 1023);

#if DEBUG_LEVEL > 2
      Serial.print("Pot_map = ");
      Serial.println(value, DEC);      
#endif
  
  if (value < 255) value = map(value, 0, 254, 1050, 1499); // exp - section 0...25%%
  if (value > 254 && value < 511) value = map(value, 255, 510, 1500, 1724); // exp - section 26...50%%
  if (value > 510 && value < 1024) value = map(value, 511, 1023, 1725, 1950); // exp - section 51...100%%
  
#if DEBUG_LEVEL > 2
      Serial.print("Pot_exp = ");
      Serial.println(value, DEC);      
#endif  
    
  return value;
}

// ------------------------------------------------------ Setup ------------------------------------------------------
// Initial settings
void setup() {

  pinMode(BLC_PIN, OUTPUT);                   // output to BLC
  digitalWrite(BLC_PIN, LOW);
  blc.attach(BLC_PIN);
  blc.writeMicroseconds(1050);                // set minimal pulse for avoid "signal lost"

  pinMode(POT_PIN, INPUT);                    // input from potentiometer
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);             // connect the Button pin to the internal resistor
  pinMode(LED_PIN, OUTPUT);                   // LED is connected to VCC with 330 Om resistor
  pinMode(INT_LED_PIN, OUTPUT);               // interlal LED on board

  LED_is_OFF();                               // LED is OFF by deffault
  int_LED_is_OFF();                           // LED is OFF by deffault
  
  Serial.begin(115200);                       // will use UART mostly for debug 

  EEPROM.get(SIGNATURE_ADDR, sign);           // get Signature from EEPROM
  if (sign != 0xAD13) {                       // is it the first start after programming?
    EEPROM.put(WORKTIME_ADDR, (int) 270);     // set working time to 4 min 30 sec as default
    EEPROM.put(MIN_TROT_ADDR, (int)0);        // min value of trottle from the potentiometer
    EEPROM.put(MAX_TROT_ADDR, (int)1023);     // max value of trottle from the potentiometer
    EEPROM.put(SIGNATURE_ADDR, (int)0xAD13);  // set signature to 0xAD13 - rev. 1.3
  };

  EEPROM.get(WORKTIME_ADDR, working_time);    // get the Working time from EEPROM
  EEPROM.get(MIN_TROT_ADDR, pot_min_value);   // min value of trottle from the potentiometer
  EEPROM.get(MAX_TROT_ADDR, pot_max_value);   // max value of trottle from the potentiometer
  EEPROM.get(SIGNATURE_ADDR, sign);           // get Signature from EEPROM

  is_button_pressed = debounce(100, 70, 10);  // 100 requests, 70 concurrences, 10 ms delay

  if (is_button_pressed) {
    operation_mode = 1;                       // ----------- go to Programming mode
    is_button_pressed = false;
  } else operation_mode = 0;                  // ----------- go to Stand-by mode

#if DEBUG_LEVEL > 0
      Serial.println("CLTimer. Rev.1.3");
      Serial.println("(c) Andrii Diagilev - dyagilev@gmail.com");
      Serial.print("FW signature: ");
      Serial.println((int)sign, HEX);
      Serial.print("Working time: ");
      Serial.println(working_time, DEC);
      Serial.print("working minutes: ");
      Serial.println((int)(working_time/60), DEC);
      Serial.print("working 10 seconds periods: ");
      Serial.println((int)((working_time % 60) / 10), DEC);
      Serial.print("Minimal trottle (DEC): ");
      Serial.println(pot_min_value, DEC);
      Serial.print("Maximal trottle (DEC): ");
      Serial.println(pot_max_value, DEC);
      Serial.println("");
#endif
  
  int_LED_is_ON();                            // setup completed
  delay(150);
  int_LED_is_OFF();
  delay(150);
 
}

// ------------------------------------------------------ Loop ------------------------------------------------------
void loop() {

  unsigned int  i, j, k, current_pulse, wrk_minutes, wrk_10seconds;
  unsigned int tmp_minutes, tmp_10seconds;         // temp value of time settings
  unsigned int tmp_pot_min_value;                           // min value from the potentiometer
  unsigned int tmp_pot_max_value;                           // max value from the potentiometer
  unsigned int delta;
  char config_mode = 1;                            // configuration step - 1 - minute setting

  switch (operation_mode) {

    case 0:                                        // --------------- Stand-by mode ---------------

      wrk_minutes = working_time / 60;
      wrk_10seconds = (working_time - (wrk_minutes * 60)) / 10;

      for (i = 0; i < wrk_minutes; i++) {          // show quantity of working minutes
        LED_is_ON();
        for (j = 0; j < 25; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
        LED_is_OFF();
        for (j = 0; j < 15; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
      };

      is_button_pressed = debounce(60, 30, 10); // 60 requests, 30 concurrences, 10 ms delay = 600 ms

      if (is_button_pressed) {
        operation_mode = 2;                     // go to Start mode
        is_button_pressed = false;
        break;
      };

      for (i = 0; i < wrk_10seconds; i++) {       // show quantity of additional 10 sec working periods
        LED_is_ON();
        for (j = 0; j < 10; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
        LED_is_OFF();
        for (j = 0; j < 10; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
      };

      if(working_time == 0){
        LED_is_ON();
        for (j = 0; j < 2; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
        LED_is_OFF();
        for (j = 0; j < 25; j++) {
          blc.writeMicroseconds(1050);
          delay(20);
        };
      };

      is_button_pressed = debounce(60, 30, 10); // 60 requests, 30 concurrences, 10 ms delay = 600 ms

      if (is_button_pressed) {
        operation_mode = 2;                     // go to Ready tro start mode
        is_button_pressed = false;
      };
      break;

    case 1:                                    // ------------------------- Programming mode -------------------------

      for (i = 0; i < 10; i++) {               // LED: On - 150 ms, Off - 150 ms - indication that Programming mode has started
        LED_is_ON();
        delay(150);
        LED_is_OFF();
        delay(150);
      };
                                                  // <--------------- insert here receiving of the configuration data via UART
      delay(500);

      wrk_minutes = working_time / 60;
      wrk_10seconds = (working_time - (wrk_minutes * 60)) / 10;
      config_mode = 1;
      tmp_minutes = wrk_minutes;
      tmp_10seconds = wrk_10seconds;

      while (true) {

        switch (config_mode) {

          case 1:                                 // --- set the minutes

            for (j = 0; j < config_mode; j++) {   // show config step
              int_LED_is_ON();
              delay(250);
              int_LED_is_OFF();
              delay(250);
            };
            
            if(tmp_minutes == 0){
              LED_is_ON();                          // show short marker if working minutes == 0
              delay(50);
              LED_is_OFF();
              delay(450);
            };
                        
            for (j = 0; j < tmp_minutes; j++) {   // show working minutes (from 1 to 9 minutes)
              LED_is_ON();
              delay(250);
              LED_is_OFF();
              delay(250);
            };

           is_button_pressed = debounce(50, 20, 10);     // 50 requests, 20 concurrences, 10 ms delay = 500 ms

            if (is_button_pressed) {
                tmp_minutes++;
                if(tmp_minutes > 9) tmp_minutes = 0;        // working time 00:10 ... 09:50
                is_button_pressed = false;
              };
            
            delay(1000);
            int_LED_is_ON();                          // show marker - be ready for settings
            delay(1000);
            int_LED_is_OFF();
            delay(1000);
        
            is_button_pressed = debounce(100, 30, 10);     // 100 requests, 30 concurrences, 10 ms delay = 1000 ms

            if (is_button_pressed) {
                EEPROM.put(WORKTIME_ADDR, (int) ((tmp_minutes * 60) + (tmp_10seconds * 10)) ); // save working time in sec.
                
#if DEBUG_LEVEL > 2                
  EEPROM.get(WORKTIME_ADDR, working_time);    // get the Working time from EEPROM
  Serial.println("Working minutes have been set!");
  Serial.print("working minutes: ");
  Serial.println((int)(working_time/60), DEC);
  Serial.print("working 10 seconds periods: ");
  Serial.println((int)((working_time % 60) / 10), DEC);
  Serial.println("");
#endif
                
                config_mode = 2;
                is_button_pressed = false;
                LED_is_ON();                               // show press confirmation
                delay(100);
                LED_is_OFF();
                delay(100);
                break;
            };
          
            break;

          case 2:                                 // --- 10 sec settings

            for (j = 0; j < config_mode; j++) {   // show config step
              int_LED_is_ON();
              delay(250);
              int_LED_is_OFF();
              delay(250);
            };
            
            if(tmp_10seconds == 0){
              LED_is_ON();                          // show short marker if working 10 seconds periods == 0
              delay(50);
              LED_is_OFF();
              delay(450);
            };
                        
            for (j = 0; j < tmp_10seconds; j++) {   // show working minutes (from 1 to 9 minutes)
              LED_is_ON();
              delay(250);
              LED_is_OFF();
              delay(250);
            };

           is_button_pressed = debounce(50, 20, 10);     // 50 requests, 20 concurrences, 10 ms delay = 500 ms

            if (is_button_pressed) {
                tmp_10seconds++;
                if(tmp_10seconds > 5) tmp_10seconds = 0;        // working time 00:10 ... 09:50
                is_button_pressed = false;
              };
            
            delay(1000);
            int_LED_is_ON();                          // show marker - be ready for settings
            delay(1000);
            int_LED_is_OFF();
            delay(1000);
        
            is_button_pressed = debounce(100, 30, 10);     // 100 requests, 30 concurrences, 10 ms delay = 1000 ms

            if (is_button_pressed) {
                EEPROM.put(WORKTIME_ADDR, (int) (tmp_minutes * 60) + (tmp_10seconds * 10) ); // save working time in sec.
                
#if DEBUG_LEVEL > 2                
  EEPROM.get(WORKTIME_ADDR, working_time);    // get the Working time from EEPROM
  Serial.println("Working 10 seconds periods have been set!");
  Serial.print("working minutes: ");
  Serial.println((int)(working_time/60), DEC);
  Serial.print("working 10 seconds periods: ");
  Serial.println((int)((working_time % 60) / 10), DEC);
  Serial.println("");
#endif                
                
                config_mode = 3;
                is_button_pressed = false;
                LED_is_ON();                               // show press confirmation
                delay(100);
                LED_is_OFF();
                delay(100);
                break;
            };          
            break;

          case 3:                                 // --- min trottle configuration

            for (j = 0; j < config_mode; j++) {   // show config step
              int_LED_is_ON();
              delay(250);
              int_LED_is_OFF();
              delay(250);
            };
                                  
            for (j = 0; j < 20; j++) {   // show inc/dec pot value
              tmp_pot_min_value = analogRead(POT_PIN);
              k = map(tmp_pot_min_value, 0, 1023, 20, 480);
              LED_is_ON();
              delay(k);
              LED_is_OFF();
              delay(500 - k);

#if DEBUG_LEVEL > 1                
  Serial.print("k = ");
  Serial.print((int)k, DEC);
  Serial.print("   potentiometer min value = ");
  Serial.println((int)tmp_pot_min_value, DEC);
  
#endif             
            };

            int_LED_is_ON();                          // show marker - be ready for settings
            delay(1000);
            int_LED_is_OFF();
            // delay(1000);
        
            is_button_pressed = debounce(100, 30, 10);     // 100 requests, 30 concurrences, 10 ms delay = 1000 ms

            if (is_button_pressed) {
                LED_is_ON();                        // show pressing confirmation
                delay(100);
                LED_is_OFF();
                delay(100);
                tmp_pot_min_value = analogRead(POT_PIN);
                EEPROM.put(MIN_TROT_ADDR, (int)tmp_pot_min_value);            // save min value of trottle from the potentiometer
                config_mode  = 4;                                             // go to min trottle configuration
                is_button_pressed = false;
              
                break;
            };          
            break;
             
          case 4:                                 // --- max trottle configuration

           for (j = 0; j < config_mode; j++) {   // show config step
              int_LED_is_ON();
              delay(250);
              int_LED_is_OFF();
              delay(250);
            };
                                  
            for (j = 0; j < 20; j++) {   // show inc/dec pot value
              tmp_pot_max_value = analogRead(POT_PIN);
              k = map(tmp_pot_max_value, 0, 1023, 20, 480);
              LED_is_ON();
              delay(k);
              LED_is_OFF();
              delay(500 - k);

#if DEBUG_LEVEL > 1                
  Serial.print("k = ");
  Serial.print((int)k, DEC);
  Serial.print("   potentiometer max value = ");
  Serial.println((int)tmp_pot_max_value, DEC);
  
#endif             
            };

            int_LED_is_ON();                          // show marker - be ready for settings
            delay(1000);
            int_LED_is_OFF();
                   
            is_button_pressed = debounce(100, 30, 10);     // 100 requests, 30 concurrences, 10 ms delay = 1000 ms

            if (is_button_pressed) {
                LED_is_ON();                        // show pressing confirmation
                delay(100);
                LED_is_OFF();
                delay(100);
                tmp_pot_max_value = analogRead(POT_PIN);
                EEPROM.put(MAX_TROT_ADDR, (int)tmp_pot_max_value);            // save max value of trottle from the potentiometer
                if (tmp_pot_max_value < tmp_pot_min_value) {                  // if used wrong potentiometer direction
                  EEPROM.put(MIN_TROT_ADDR, (int)tmp_pot_max_value);
                  EEPROM.put(MAX_TROT_ADDR, (int)tmp_pot_min_value);
                };
                config_mode  = 1;                   // go to minutes setup step
                is_button_pressed = false;
                delay(1000);
                break;
            };          
     
        break;

        };
      };    

    case 2:                                         // --------------- Ready to start ---------------

      for (i = 0; i < (START_DELAY / 160); i++) {   // LED: On - 100 ms, Off - 100 ms / ???? Q = 8 MHz ????
        LED_is_OFF();
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
        LED_is_ON();
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
        blc.writeMicroseconds(1050);
        delay(20);
      };

      pulse_width = get_ppm_mks(POT_PIN);
      operation_mode = 3;                           // go to Start mode
      break;

    case 3:                                         // --------------- Start mode (smooth start of the motor) ---------------

      delta = pulse_width - 1050;
      j = 1;
      k = 1;

#if DEBUG_LEVEL > 2
      Serial.print("pulse_width = ");
      Serial.println(pulse_width, DEC);
      Serial.print("delta = ");
      Serial.println(delta, DEC);
#endif

      for (i = 0; i < (START_TIME / 20); i++) {   // number of 20 ms periods
        current_pulse = (unsigned int) ((((unsigned long) delta * i * 20) / (unsigned int) START_TIME) + 1050);
        blc.writeMicroseconds(current_pulse);

#if DEBUG_LEVEL > 2
        Serial.print("i = ");
        Serial.println(i, DEC);
        Serial.print("current_pulse = ");
        Serial.println(current_pulse, DEC);
#endif

        if (i % 10) {                              // show start mode increasing LED duty cycle
          LED_is_ON();

          if (j == k) {
            LED_is_OFF();
            k++;
            j = 0;
          } else {
            j++;
          };
        };
        delay(20);
      };

      operation_mode = 4;                         // go to Working mode
      break;

    case 4:                                        // --------------- Working mode ---------------

      LED_is_ON();

      for (i = 0; i < (working_time * (1000 / 20)); i++) {      // count the number of 20 ms cycles within working time (sec * 1000 / 20 ms)
        pulse_width = get_ppm_mks(POT_PIN);                     // you can tune throttle at this moment
        blc.writeMicroseconds(pulse_width);
        delay(20); //
      };

      operation_mode = 5;                         // go to Stop mode
      break;

    case 5:                                       // --------------- Stop mode (smooth stop of the motor) ---------------

     delta = pulse_width - 1050;
      j = 1;
      k = 1;

#if DEBUG_LEVEL > 2
      Serial.print("pulse_width = ");
      Serial.println(pulse_width, DEC);
      Serial.print("delta = ");
      Serial.println(delta, DEC);
#endif

      for (i = (START_TIME / 20); i > 0; i--) {   // number of 20 ms periods
        current_pulse = (unsigned int) ((((unsigned long) delta * i * 20) / (unsigned int) START_TIME) + 1050);
        blc.writeMicroseconds(current_pulse);

#if DEBUG_LEVEL > 2
        Serial.print("i = ");
        Serial.println(i, DEC);
        Serial.print("current_pulse = ");
        Serial.println(current_pulse, DEC);
#endif

        if (i % 5) {                              //  show stop mode decreasing LED duty cycle
          LED_is_OFF();

          if (j == k) {
            LED_is_ON();
            k++;
            j = 0;
          } else {
            j++;
          };
        };
        delay(20);
      };

      operation_mode = 0;                         // go to Stand-By mode
      break;
  };

}
