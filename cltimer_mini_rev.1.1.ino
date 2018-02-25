// CL Timer - Timer for Indoor Control Line models with BLC/BC controllers
// Rev. 1.1
// (c) Andrii Diagilev - dyagilev@gmail.com

#include <Servo.h>
#include <EEPROM.h>    

// Timing settings
#define START_DELAY  3000 // ms - задержка от момента нажатия кнопки до запуска двигателя
#define START_TIME   3000 // ms - время, за которое происходит увеличение оборотов двигател от нуля до установленного максимума
#define STOP_TIME    3000 // ms - время, за которое происходит уменьшение оборотов двигателя от установленного максимума до нуля

// Hardware connections
#define BUTTON_PIN   10 // 14 PB2 - 10 - OC1B - PCINT2 - PWM - SS
#define POT_PIN      A0 // 23 PC0 - 14 A0 - PCINT8 - ADC0
#define LED_PIN       7 // 11 PD7 - 7 - AIN1 - PCINT23
#define BLC_PIN       2 // 32 PD2 - 2 - INT0 - PCINT18

#define WORKTIME_ADDR 0 // EEPROM address for Working Time value

Servo blc;

int operationmode = 0; // Stand-by mode by default
int potvalue = 0; // analog value from potentiometer
boolean ledison = false; // False - OFF, True- ON
boolean isbuttonpressed = false; // False - not pressed, True - pressed
int pulsewidth = 0; // blc pulse width (0...180 degrees) set to 0;
int workingtime;    // working time in sec.

// ------------------------------------------------------
// Read the Button state with Debounce - защита от дребезга контактов

boolean debounce(int polls, int concurrences, int reqdelay) { 

// polls - number of button polls - количество опросов кнопки
// concurrences - number of required concurrences for debounce - количество запросов, в которых кнопка была нажата 
// reqdelay - delay between the polls, delay = polls x reqdelay - задержка между запросами
// return: true - if the Button has been pressed - если считаем, что кнопка была нажата
  
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
  value = map(value, 0, 1023, 1050, 1950); // пересчет показаний потенциометра в длину импульса  

 return value;
}


// ------------------------------------------------------
// Initial settings
void setup() {
  
  pinMode(BLC_PIN, OUTPUT);
  digitalWrite(BLC_PIN, LOW);  
  blc.attach(BLC_PIN); 
  blc.writeMicroseconds(1050); // stop the motor

  pinMode(POT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH); // connect the Button pin to the internal resistor
  pinMode(LED_PIN, OUTPUT); // LED is connected to VCC with 330 Om resistor
  digitalWrite(LED_PIN, HIGH); // LED is OFF by deffault
    
  EEPROM.get(WORKTIME_ADDR, workingtime); // get the Working time from EEPROM - получить запрограммированную продолжительность работы работы из EEPROM

  isbuttonpressed = debounce(100, 70, 10); // 100 requests, 70 concurrences, 10 ms delay - 100 запросов за 1 секунду, 70 совпадений

      if (isbuttonpressed){
        operationmode = 1; // go to Programming mode - переход в Режим Программирования
        isbuttonpressed = false;        
      };
}

// ------------------------------------------------------
void loop() {

  unsigned int  i, j, currentpulse; 
  int delta;

  switch (operationmode) { // переключатель рабочих режимов
    
    case 0: // Stand-by mode - Режим Готовности

      for(i = 0; i < (workingtime / 10); i++){ // показываем запрограммированое рабочее время (1 вспышка = 10 секунд)
        digitalWrite(LED_PIN, LOW); // LED is ON
        for (j = 0; j < 30; j++){
          blc.writeMicroseconds(1050);
          delay(10);
        };
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        for (j = 0; j < 40; j++){
          blc.writeMicroseconds(1050); // отправляем сигнал на контролер, чтобы небыло срабатывания по потере сигнала           
          delay(10);
        };
       };
      
      isbuttonpressed = debounce(60, 40, 10); // 60 requests, 40 concurrences, 10 ms delay - 0.6 секунды, 60 запросов, 40 совпадений
      
      if (isbuttonpressed){
        operationmode = 2; // go to Start mode - переходим в Стартовый режим
        isbuttonpressed = false;        
      };
      break;
    
    case 1: // Programming mode - Режм Программирования

      for (i = 0; i < (START_TIME / 300); i++){       // LED: On - 150 ms, Off - 150 ms - моргаем, чтобы показать, что зашли в Режим Программирования
        digitalWrite(LED_PIN, HIGH); // LED is OFF
        delay(150);
        digitalWrite(LED_PIN, LOW); // LED is ON
        delay(150);        
      };
      
      while(true){                                    // отображаем текуще запрограммированное время и ждем нажатия кнопки 
        for(i = 0; i < (workingtime / 10); i++){
          digitalWrite(LED_PIN, LOW); // LED is ON
          delay(250); 
          digitalWrite(LED_PIN, HIGH); // LED is OFF
          delay(500);             
        };

        delay (300);
          isbuttonpressed = debounce(30, 20, 10); // 60 requests, 40 concurrences, 10 ms delay = 600 ms
        delay (500);
      
        if (isbuttonpressed){ // если кнопка нажата, добавляем 10 секунд к рабочему времени
          workingtime += 10; // increase working time for 10 sec
          if(workingtime > 60) workingtime = 10; // если получили > 60 секунд, ставим 10 секунд рабочего времени
          isbuttonpressed = false;
          EEPROM.put(WORKTIME_ADDR, workingtime);  // write Working Time value to EEPROM  - сохраняем новую продолжительность рабочего времени в EEPROM        
         };
      };
    break;
    
    case 2: // Ready to start - Режим Готовности к Старту
      
      for (i = 0; i < (START_DELAY / 300); i++){ // LED: On - 150 ms, Off - 150 ms
       digitalWrite(LED_PIN, HIGH); // LED is OFF
       blc.writeMicroseconds(1050);
       delay(150);
       digitalWrite(LED_PIN, LOW); // LED is ON
       blc.writeMicroseconds(1050);
       delay(150);        
      };
      
      pulsewidth = get_ppm_mks(POT_PIN);  // получаем текущее значение "газа", установленное потенциометром 
      operationmode = 3; // go to Start mode - переходим к Режиму Старта
      break;

    case 3: // Start mode - Режим Старта - плавно раскручиваем двигатель
    
        delta = pulsewidth - 1050;
        for(i = 0; i < 30; i++){
          currentpulse = 1050 + (delta * i) / 30;
          blc.writeMicroseconds(currentpulse);
          if(i % 3){
            digitalWrite(LED_PIN, LOW); // LED is ON 
          } else {
            digitalWrite(LED_PIN, HIGH); // LED is OFF 
          };
            
        delay(START_TIME / 30);
      };

      operationmode = 4; // go to Working mode - переходим в Режим Работы
      break;

    case 4: // working mode - Рабочий Режим

      digitalWrite(LED_PIN, LOW);               // LED is ON - светодиод горит постоянно в тесение Рабочего времени
      
     for(i = 0; i < (workingtime * 20); i++){
        pulsewidth = get_ppm_mks(POT_PIN);          // you can tune throttle at this moment - в Рабочем режиме можно регулировать"газ" 
        blc.writeMicroseconds(pulsewidth);   
        delay(25); // 
        blc.writeMicroseconds(pulsewidth);
        delay(25); // 
      };
       
      operationmode = 5; // go to Stop mode
      break;

    case 5: // Stop mode - Режим Остановки

        delta = pulsewidth - 1050;
        for(i = 0; i < 30; i++){                // плавное уменьшение от установленого максимума до нуля
          currentpulse = pulsewidth - (delta * i) / 30;
          blc.writeMicroseconds(currentpulse);
          if(i % 3){
            digitalWrite(LED_PIN, HIGH); // LED is ON 
          } else {
            digitalWrite(LED_PIN, LOW); // LED is OFF 
          };
            
        delay(START_TIME / 30);
      };

      operationmode = 0; // go to Stand-By mode - переходим в Режим Готовности
      break;
  }
  
}
