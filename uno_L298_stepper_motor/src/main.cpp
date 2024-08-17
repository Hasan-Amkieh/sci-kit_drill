#include <Arduino.h>
#include <Stepper.h>
#include <HX711.h>

// The stepper motor settings are max current and 1600 pulses per revolution, the used controller is TB6600
// The DC motor driver is BTS7960b
// The stepper motor driver is L298N as a steppr motor driver

// 6.5 cm down for 4000
// 7.5 cm upward for 5000
// 1.1 cm for around 1000 pulses

#define DOUT  3
#define CLK  2
#define DRILL_NORMAL_SPEED 255 // max is 255 and min is 0
#define CAROUSEL_HALL_SENSOR A0
#define ELEVATOR_DISTANCE_SENSOR A1
#define DRILL_CURRENT_SENSOR A2

// Number of steps per output rotation
const int carouselStepsPerRevolution = 200;

Stepper carouselStepper(carouselStepsPerRevolution, 4, 5, 6, 7);

HX711 scale(DOUT, CLK);

float last_weight;
float sample_weights[4] = {0, 0, 0, 0};
int sample_index = 0;

float calibration_factor = 419640; // for me this value works just perfect 419640

int pwmA = 10;
int in1A = 9;
int in2A = 8;

const int stepPin = 12; 
const int dirPin = 11;
const int enPin = 13;

bool isCarouselFull = false;

void moveCarousel() {
	for (int i = 0 ; i < 50 ; i++) {
		Serial.println(analogRead(CAROUSEL_HALL_SENSOR));
		if (analogRead(CAROUSEL_HALL_SENSOR) >= 540) {
			Serial.println("Magnet detected!");
			carouselStepper.step(60);
			delay(1000);
			break;
		} else {
			carouselStepper.step(2);
		}
		delay(50);
	}
}

void sendElevatorPulse() {

	digitalWrite(stepPin,HIGH); 
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);

}

void checkPayloadWeight() {

	float weight = scale.get_units(5);
	if (weight < 0) {
    	weight = 0.00;
	}
	
	if (sample_index < 3 && (weight - last_weight) >= 0.02) {
		sample_weights[sample_index] = weight - last_weight;
		last_weight = weight;
		sample_index++;
		analogWrite(pwmA, 0);
		moveCarousel();
		analogWrite(pwmA, DRILL_NORMAL_SPEED);
	} else if (sample_index == 3 && (weight - last_weight) >= 0.02) {
		sample_weights[sample_index] = weight - last_weight;
		last_weight = weight;
		sample_index++;
		isCarouselFull = true;
		analogWrite(pwmA, 0);
	}

}

int getElevatorHeight() {


	int cms = 0;
	for (int i = 0 ; i < 5 ; i++) {
		cms += (6762 /(analogRead(ELEVATOR_DISTANCE_SENSOR)-9)) -(4);
		delay(10);
	}

	cms = cms / 5;
	Serial.println(cms);
	return cms;

}

float get_drill_current () {
	return 5 / 1023.0 * analogRead(DRILL_CURRENT_SENSOR) - 2.48;
}

void setup() {
	carouselStepper.setSpeed(30);
	pinMode(CAROUSEL_HALL_SENSOR, INPUT);
	pinMode(ELEVATOR_DISTANCE_SENSOR, INPUT);
	pinMode(DRILL_CURRENT_SENSOR, INPUT);

	pinMode(pwmA, OUTPUT);
	pinMode(in1A, OUTPUT);
	pinMode(in2A, OUTPUT);

	pinMode(stepPin,OUTPUT);
	pinMode(dirPin,OUTPUT);
	pinMode(enPin,OUTPUT);
	digitalWrite(enPin,LOW);

	digitalWrite(in1A, HIGH);
	digitalWrite(in2A, LOW);
	analogWrite(pwmA, DRILL_NORMAL_SPEED);

	scale.set_scale();
	scale.tare();
	scale.set_scale(calibration_factor);

	Serial.begin(9600);
	while (!Serial) ;

	last_weight = scale.get_units(5);
	if (last_weight < 0) {
    	last_weight = 0.00;
	}

	moveCarousel(); // move the carousel to one of the cups, as the filling might start any moment
	
	// LOW is UPWARDS FOR ELEVATOR
	digitalWrite(dirPin,HIGH);
	while (getElevatorHeight() > 12) {
		for (int x = 0; x < 200; x++) {
    		sendElevatorPulse();
  		}
		checkPayloadWeight();
		Serial.print(get_drill_current());
		Serial.println(" Amps");
	}
	Serial.println("Bottom is reached!");

	for (int i = 0 ; i < 5000 ; i++) {
		if (!isCarouselFull) {
			checkPayloadWeight();
		} else {
			break;
		}
		Serial.print(get_drill_current());
		Serial.println(" Amps");
		delay(100);
	}

	digitalWrite(dirPin,LOW);
	while (getElevatorHeight() < 30) {
		for (int x = 0; x < 200; x++) {
    		sendElevatorPulse();
  		}
		Serial.print(get_drill_current());
		Serial.println(" Amps");
		if (!isCarouselFull) {
			checkPayloadWeight();
			delay(100);
		}
	}

	digitalWrite(in1A, LOW);
	digitalWrite(in2A, HIGH);
	analogWrite(pwmA, DRILL_NORMAL_SPEED);
	delay(10000);
	analogWrite(pwmA, 0);

}

void loop() {
	;

}