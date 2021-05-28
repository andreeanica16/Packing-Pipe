#include <Servo.h>

#define FASTLED_INTERNAL
#include <FastLED.h>

#define LED_PIN     12
#define NUM_LEDS    17

#define FIRST_SERVO_PIN 9
#define SECOND_SERVO_PIN 10

#define BUZZER_PIN 3

// TCS230 or TCS3200 pins wiring to Arduino
#define S0 4
#define S1 5
#define S2 7
#define S3 6
#define sensorOut 8

#define MAX_TIMER_COUNT 280

#define STATE_NUMBERS 6

#define START_STATE 0
#define POSITION_TO_SENSOR_STATE 1
#define SENSOR_STATE 2
#define POSITION_COLOR_PACK_STATE 3
#define POSITION_OBJECT_FINAL_STATE 4
#define FINAL_STATE 5

#define START_POSITION 0
#define SENSOR_POSITION 79
#define SORTER_POSITION 156

#define NO_COLOR_POSITION 0
#define RED_POSITION 45
#define BLUE_POSITION 90
#define GREEN_POSITION 135
#define YELLOW_POSITION 180

#define RED 0
#define BLUE 1
#define GREEN 2
#define YELLOW 3

volatile int timer_count = 0;
volatile int is_state = 0;

int state = START_STATE;
int toggle = LOW;
int color;

// Stores frequency read by the photodiodes
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

CRGB leds[NUM_LEDS];


Servo firstServo;
Servo secondServo;

ISR(TIMER2_COMPA_vect) {
  timer_count++;
  if (timer_count >= MAX_TIMER_COUNT) {
    // reset the timer count
    timer_count = 0; 
    is_state = 1;
  }
  
}

void setupTimer() {
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  
  OCR2A = 222; // compare match register 16MHz/1024/70Hz-1
  TCCR2A |= (1 << WGM21);   // CTC mode
  TCCR2B |= (1 <<  CS22) | (1 <<  CS21) |(1 << CS20);    // 1024 prescaler 
  TIMSK2 |= (1 << OCIE2A);  // enable timer compare interrupt
}

void setUpSensor() {
  // Setting the outputs
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  
  // Setting the sensorOut as an input
  pinMode(sensorOut, INPUT);
  
  // Setting frequency scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);
}

void setup() {
 // configurati pin-ul de output
  pinMode(13, OUTPUT);

 // configuram Buzzer
 pinMode (BUZZER_PIN, OUTPUT);
 digitalWrite (BUZZER_PIN, HIGH);
 
 // configurare servomotoare
   firstServo.attach(FIRST_SERVO_PIN);
   secondServo.attach(SECOND_SERVO_PIN);
 
 // init LED Strip
 FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

 // init Color Sensir
 setUpSensor();

 // Begins serial communication 
  Serial.begin(9600);
  
 // initializare timer 2
  cli();
  setupTimer();
  sei();
}

void colorLedStrip(int red, int green, int blue) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB( red, green, blue);
    FastLED.show();
    delay(40);
  }
}

void colorLedStripWithRecognisedColor() {
  switch (color) {
    case RED:
      colorLedStrip(255, 0, 0);
      break;
    case BLUE:
      colorLedStrip(0, 0, 255);
      break;
    case GREEN:
      colorLedStrip(0, 255, 0);
      break;
    case YELLOW:
      colorLedStrip(255, 255, 0);
      break;
    default:
      colorLedStrip(0, 0, 0);
  }
}

int readRedFrequency() {
  int result;
  
  // Setting RED (R) filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  
  // Reading the output frequency
  result = pulseIn(sensorOut, LOW);
  return result;
}

int readGreenFrequency() {
  int result;

  // Setting GREEN (G) filtered photodiodes to be read
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  
  // Reading the output frequency
  result = pulseIn(sensorOut, LOW);
  return result;
}

int readBlueFrequency() {
  int result;
  
  // Setting BLUE (B) filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  
  // Reading the output frequency
  result = pulseIn(sensorOut, LOW);
  return result;
}

void readColor() {
  // Printing the RED (R) value
  redFrequency = readRedFrequency();
  Serial.print("R = ");
  Serial.print(redFrequency);
  delay(100);

  // Printing the GREEN (G) value
  greenFrequency = readGreenFrequency();
  Serial.print(" G = ");
  Serial.print(greenFrequency);
  delay(100);

  // Printing the BLUE (B) value 
  blueFrequency = readBlueFrequency();
  Serial.print(" B = ");
  Serial.println(blueFrequency);
  delay(100);
  
  color = -1;
  
  if (
    redFrequency  >= 44 && redFrequency <= 59 && 
    greenFrequency >= 42 && greenFrequency <= 50 && 
    blueFrequency <= 30) {
    color =  BLUE;
  }

  if (
    redFrequency  >= 25 && redFrequency <= 40 && 
    greenFrequency >= 45 && greenFrequency <= 60 && 
    blueFrequency >= 30 && blueFrequency <= 45) {
    color = RED;
  }

  if (
    redFrequency  >= 35 && redFrequency <= 45 && 
    greenFrequency >= 38 && greenFrequency <= 45 && 
    blueFrequency >= 30 && blueFrequency <= 40) {
    color = GREEN;
  }

   if (
    redFrequency  >= 20 && redFrequency <= 32 && 
    greenFrequency >= 30 && greenFrequency <= 40 && 
    blueFrequency >= 20 && blueFrequency <= 40) {
    color = YELLOW;
  }  
}


void buzzer_beep() {
  for (int i = 0; i < 60; i++) {
    digitalWrite (BUZZER_PIN, LOW);
    delay (4);
    digitalWrite (BUZZER_PIN, HIGH);
    delay (1);
  }
}

void rotateSecondServoMotor(int startP, int endP, int stepP) {
  for (int i = startP; i != endP; i += stepP) {
    secondServo.write(i);
    delay(15);
  }
  delay(15);
}

void rotateFirstServoMotor(int startP, int endP, int stepP) {
  for (int i = startP; i != endP; i += stepP) {
    firstServo.write(i);
    delay(15);
  }
  delay(15);
}

void loop() {
  if (is_state == 1) {
    is_state = 0;
    
    toggle = toggle == LOW ? HIGH : LOW;
    digitalWrite(13, toggle);
    
    if (state == START_STATE) {
      firstServo.write(START_POSITION);
      secondServo.write(NO_COLOR_POSITION);
      colorLedStrip(0, 0, 0);
    } else if (state == POSITION_TO_SENSOR_STATE) {
      rotateFirstServoMotor(START_POSITION, SENSOR_POSITION, 1);
    } else if (state == SENSOR_STATE) {
      delay(50);
      readColor();
      colorLedStripWithRecognisedColor();
    } else if (state == POSITION_COLOR_PACK_STATE) {
      switch(color) {
        case RED:
          rotateSecondServoMotor(NO_COLOR_POSITION, RED_POSITION, 1);
          break;
        case BLUE:
          rotateSecondServoMotor(NO_COLOR_POSITION, BLUE_POSITION, 1);
          break;
        case GREEN:
          rotateSecondServoMotor(NO_COLOR_POSITION, GREEN_POSITION, 1);
          break;
        case YELLOW:
          rotateSecondServoMotor(NO_COLOR_POSITION, YELLOW_POSITION, 1);
          break;
        default:
          buzzer_beep();
          buzzer_beep();
          buzzer_beep();
          secondServo.write(NO_COLOR_POSITION); 
      }
    } else if (state == POSITION_OBJECT_FINAL_STATE) {
      rotateFirstServoMotor(SENSOR_POSITION, SORTER_POSITION, 1);
      delay(50);
    } else if (state == FINAL_STATE) {
      rotateFirstServoMotor(SORTER_POSITION, START_POSITION, -1);
      switch(color) {
        case RED:
          rotateSecondServoMotor(RED_POSITION, NO_COLOR_POSITION, -1);
          break;
        case BLUE:
          rotateSecondServoMotor(BLUE_POSITION, NO_COLOR_POSITION, -1);
          break;
        case GREEN:
          rotateSecondServoMotor(GREEN_POSITION, NO_COLOR_POSITION, -1);
          break;
        case YELLOW:
          rotateSecondServoMotor(YELLOW_POSITION, NO_COLOR_POSITION, -1);
          break;
        default:
          secondServo.write(NO_COLOR_POSITION); 
      }
    }
    
    state = (state + 1) % STATE_NUMBERS;
  }
}
