boolean ledState;
int indicationLED = 13; //Corresponds to the led marked "L"

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
}      

String readString;
void loop()
{
  if(Serial.available()) {
    switch(Serial.read()) {
      case 'a':
        Serial.print("that was an A!\n");
        break;
      case 'b':
        if(ledState == HIGH)
          ledState = LOW;
        else
          ledState = HIGH;
        digitalWrite(indicationLED, ledState);
        break;
      default:
        Serial.print("That i dont know what it was");
    }
    Serial.flush();
  }
  delay(10);
}

