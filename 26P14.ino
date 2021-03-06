#include <Servo.h> 

/////////////////////////////
// Configurable parameters //
/////////////////////////////

// Arduino pin assignment
#define PIN_LED 9
#define PIN_SERVO 10
#define PIN_IR A0

// Framework setting
#define _DIST_TARGET 255
#define _DIST_MIN 100
#define _DIST_MAX 410

// Distance sensor
#define _DIST_ALPHA 0.35

// Servo range
#define _DUTY_MIN 1125
#define _DUTY_NEU 1475
#define _DUTY_MAX 1825

// Servo speed control
#define _SERVO_ANGLE 30
#define _SERVO_SPEED 600

// Event periods
#define _INTERVAL_DIST 20
#define _INTERVAL_SERVO 20
#define _INTERVAL_SERIAL 100 

#define DELAY 1500

// PID parameters
#define _KP 0.6
#define _KD 18.0

#define a 70
#define b 417

//////////////////////
// global variables //
//////////////////////

// Servo instance
Servo myservo;

// Distance sensor
float dist_target; // location to send the ball
float dist_raw;
float dist_ema=0; //측정된 값과 ema 필터를 적용한 값
float samples_num = 3;  

// Event periods
unsigned long last_sampling_time_dist, last_sampling_time_servo, last_sampling_time_serial; 
bool event_dist, event_servo, event_serial; 

// Servo speed control
int duty_chg_per_interval;
int duty_target, duty_curr;

// PID variables
float error_curr, error_prev, control, pterm, dterm, iterm;

void setup() {
// initialize GPIO pins for LED and attach servo 
myservo.attach(PIN_SERVO);

pinMode(PIN_LED,OUTPUT);

// initialize global variables
duty_curr = _DUTY_NEU;

// move servo to neutral position
myservo.writeMicroseconds(_DUTY_NEU);

// initialize serial port
Serial.begin(57600);

// convert angle speed into duty change per interval.
  duty_chg_per_interval = (_DUTY_MAX - _DUTY_MIN) * (_SERVO_SPEED / _SERVO_ANGLE ) * (_INTERVAL_SERVO / 1000.0);
}
  

void loop() {
/////////////////////
// Event generator //
/////////////////////

unsigned long time_curr = millis();
if(time_curr >= last_sampling_time_dist + _INTERVAL_DIST){
    last_sampling_time_dist += _INTERVAL_DIST;
    event_dist = true;
}

if(time_curr >= last_sampling_time_servo + _INTERVAL_SERVO ){
    last_sampling_time_servo += _INTERVAL_SERVO;
    event_servo = true;
}

if(time_curr >= last_sampling_time_serial + _INTERVAL_SERIAL ){
    last_sampling_time_serial += _INTERVAL_SERIAL;
    event_serial = true;
}

////////////////////
// Event handlers //
////////////////////

  // get a distance reading from the distance sensor
  if(event_dist) {
     event_dist = false;
     dist_raw = ir_distance_filtered();

  // PID control logic
    error_curr = _DIST_TARGET - dist_raw;
    pterm = _KP * error_curr;
    iterm = 0; 
    dterm = _KD * (error_curr - error_prev);
    control = pterm + dterm + iterm;
    
  // duty_target = f(duty_neutral, control)
    duty_target = _DUTY_NEU + control;

  // keep duty_target value within the range of [_DUTY_MIN, _DUTY_MAX]
    if(duty_target > _DUTY_MAX){
      duty_target = _DUTY_MAX;
    }
    else if(duty_target < _DUTY_MIN){
      duty_target = _DUTY_MIN;
    }

  // update error_prev
    error_prev = error_curr;
  }
  
  if(event_servo) {
    event_servo=false;
    // adjust duty_curr toward duty_target by duty_chg_per_interval
    if(duty_target>duty_curr) {
      duty_curr += duty_chg_per_interval;
      if(duty_curr > duty_target) duty_curr = duty_target;
    }
    else {
      duty_curr -= duty_chg_per_interval;
      if(duty_curr < duty_target) duty_curr = duty_target;
    }
    // update servo position
     myservo.writeMicroseconds(duty_curr);
  }   

  if(event_serial) {
    event_serial = false; 
    Serial.print("dist_ir:");
    Serial.print(dist_raw);
    Serial.print(",pterm:");
    Serial.print(map(pterm,-1000,1000,510,610));
    Serial.print(",dterm:");
    Serial.print(map(dterm,-1000,1000,510,610));
    Serial.print(",duty_target:");
    Serial.print(map(duty_target,1000,2000,410,510));
    Serial.print(",duty_curr:");
    Serial.print(map(duty_curr,1000,2000,410,510));
    Serial.println(",Min:100,Low:200,dist_target:255,High:310,Max:410");
  }
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return 300.0 / (b - a) * (val - a) + 100;
}

float noise_filtered(void){
  int currReading;
  int largestReading = 0;
  for (int i = 0; i < samples_num; i++) {
    currReading = ir_distance();
    if (currReading > largestReading) { largestReading = currReading; }
    delayMicroseconds(DELAY);
  }
  return largestReading;
}

float ir_distance_filtered(void){
  int currReading;
  int lowestReading = 1024;
  dist_raw = ir_distance(); 
  for (int i = 0; i < samples_num; i++) {
    currReading = noise_filtered();
    if (currReading < lowestReading) { lowestReading = currReading; }
  }
  dist_ema = _DIST_ALPHA *lowestReading + (1-_DIST_ALPHA )*dist_ema;
  return dist_ema;
}
