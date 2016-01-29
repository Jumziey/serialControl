#include <Servo.h>

boolean ledState;
int indicationLED = 13; //Corresponds to the led marked "L"
Servo servoLeft;
Servo servoRight;

void setup()
{
  boolean noContact;
  
  pinMode(indicationLED, OUTPUT);

  // google "arduino serial"
  Serial.begin(9600);
  
  //establish contact
  noContact = true;
  while(noContact) { 
  if(Serial.available())
    if(Serial.read() == '#') {
      noContact = false;
      Serial.write("C");
      ledState = HIGH;
      digitalWrite(indicationLED,ledState);
    }
   delay(10);
  }
  //For servo example
  servoLeft.attach(6);
  servoRight.attach(5);
}      

String readString;
void loop()
{
  if(Serial.available()) {
    switch(Serial.read()) {
      //All buttons
      case 'b': 
      	switch(Serial.read()) {
		      case '0':
		      		//Whatever button 0 should do
		      		//Recommend Serial.parseInt() to see if its 1(buttonpress) or 0(buttonrelease)
		      		break;
		      	case '1':
		      		//Whatever button 1 should do... etc
		      		//Recommend Serial.parseInt() to see if its 1(buttonpress) or 0(buttonrelease)
		      		break;
		      	default:;
		    }
		  //All axes except shoudlers
      case 'a': 
      	switch(Serial.read()) {
      		case '0':
      			//Whatever axis 0 should do
      			//Recommend Serial.parseInt() to read numerical value
        		break;
        	case '1':
        		//Whatever axis 1 should do... etc
        		//Recommend Serial.parseInt() to read numerical value
        		break;
        	default:;
      	}
      //Everything  that has to do with shoulder axis to do
      case 's': 
        switch(Serial.read()) {
        	case '0':
        		servoLeft.write(Serial.parseInt()/2+5);
        		break;
        	case '1':
        		servoRight.write(Serial.parseInt()/2+5);
        		break;
        	default:;
        }
        
      default:;
    }
    Serial.flush();
  }
  delay(10);
}
