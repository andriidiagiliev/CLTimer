# CLTimer
Timer for Indoor Control Line models with BLC/BC controllers

This Timer is designed to be used with the motor controllers installed on the Control Line models
As a motherboardгыув the clone of the Arduino Pro Mini (Vcc 5V received from BLC controller)

To control the timer has been used:
- potentiometer - for setting the maximum value of the trottle
- button - to switch operaton modes
For the indication of the operating modes the LED has been used.

Basic timer states
- Working mode
- Programming mode

For the entering to the Programming mode, press and hold the button and turn on the power
Programming of the timer's working time is carried out by pressing the button, the setting step is 10 seconds, you can set the time period from 10 seconds to 10 minutes 50 seconds.

Attention!!! For correct operation it is necessary to enter the programming mode to set the default value - 20 seconds at the first start after the assembly 

In the operating mode there are the following conditions:
- Stand-by state - the Timer goes into this mode when power is turned on. The flash of the LED indicates the programmed time. If in this mode, press and hold the button for about one second, the Timer will go into the Preparation for start state
- Preparation to start state is indicated by frequent flashes of the LED, the duration of the state (START_DELAY) is 3 seconds, at the end of this period the Timer will go into the Start state
- Start state- the smooth increasing of the throttle position from the zero state to the maximum value (which has been setted by the potentiometer) during (START_TIME) 3 seconds. After the expiration of this time, the Operation time state automatically starts
- Operating time - the throttle position value is stored at the maximum level for the programmed time. At the end of this period, the Stop state automatically starts
- Stop state - smooth decrease of the throttle value from the maximum to the zero state during (STOP_TIME) - 3 sec. After the end of this time, the timer will again switch to the Stand-by state

comment
electrical cirquit, a list of elements, photos of the assembled Timer will be upload additionally

rev.1.1. - Timer for models like F2A Speed (you can set operation time from 10 to 60 seconds)

rev.1.2. - Timer for common use (you can set operation time from 10 seconds to 10 minutes and 50 seconds) - do not use!

rev.1.3. - reliase version. Operation time from 10 seconds up to 9 min 50 sec + user manual (rus)

