/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
*/
int pressed = 0;  // current button

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  int b;
  // read the input on analog pin 0:
  int sensorValue = analogRead(A1);
  // print out the value you read:
  if (((b = button(sensorValue)) != 0) && (b != pressed)) {
    Serial.print("Button ");
    Serial.print(b);
    Serial.println(" pressed");
    pressed = b;
  }
  else if ((b == 0) && (pressed != 0)) {
     Serial.print("Button ");
    Serial.print(pressed);
    Serial.println(" released");
    pressed = 0;
  }
  delay(50);  // delay in between reads for stability
}

int button(int sensorValue)
{
  int n = (1023 - sensorValue)>>3;
  int b = 0;
  // Serial.println(n);
  switch(n) {
  case 2:  b = 1; break;
  case 5:  b = 2; break;
  case 7:  b = 3; break;
  case 10: b = 4; break;
  default: break;
  }
  return b;
}