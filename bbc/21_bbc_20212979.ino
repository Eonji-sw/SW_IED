#include <Servo.h>
#define PIN_SERVO 10
#define PIN_IR A0
#define IR_PIN A0
#include "medianfilter.h"

MedianFilter<> filter;

Servo myservo;

int a, b; // unit: mm

void setup() {
// initialize GPIO pins
  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(1475);
// initialize serial port
  Serial.begin(57600);

  filter.init();
  
  a = 72;
  b = 280;
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return val;
}

void loop() {
  if(filter.ready()) {
    short dist = filter.read();
    Serial.print("min:0, max:50, dist:");
    Serial.println(dist);
  }
  
  float raw_dist = ir_distance();
  float dist_cali = 100 + 300.0 / (b - a) * (raw_dist - a);
  Serial.print("min:100,max:450,dist:");
  Serial.print(raw_dist);
  Serial.print(",dist_cali:");
  Serial.println(dist_cali);
  
  if(dist_cali > 255) {
    myservo.writeMicroseconds(1274);
  }
  else {
    myservo.writeMicroseconds(1700);
  }
  delay(20);
}
