




const int pinButton = 2;
int signals[] = {A2, A3, A4};
bool button_pressed[] = {false, false, false};
int actions[] = {MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE};
#define DEADZONE 100.0
#define SPEED 30.0

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(signals[i], INPUT_PULLUP); // pas besoin de resistances pour les switchs.
  }
  Serial.begin(9600);
  Serial.println("start!!!");

  // Sends a clean report to the host. This is important on any Arduino type.
  Mouse.begin();
}



void loop() {  
  if (!digitalRead(pinButton)) {    
    for (int i = 0; i < 3; i++) {
      if (not digitalRead(signals[i]) and not button_pressed[i]) {
        button_pressed[i] = true;
        Mouse.press(actions[i]);
        Serial.print("press: ");
        Serial.print(i);
        Serial.print("\n");
      }

      if (digitalRead(signals[i]) and button_pressed[i]) {
        button_pressed[i] = false;
        Mouse.release(actions[i]);
        Serial.print("release: ");
        Serial.print(i);
        Serial.print("\n");
    
      }
    }

    float dx = -(analogRead(A0) - 1024/2) ;
    float dy = -(analogRead(A1) - 1024/2) ;
    if (abs(dx) <= DEADZONE ) {dx = 0.0;}
    if (abs(dy) <= DEADZONE ) {dy = 0.0;}
    // change with a button press or something
    if (false) {
      Mouse.move(dx/1024 * SPEED, dy/1024 * SPEED);
    }
    else {
      
//      Serial.print("scrolling");
      if (dy > 0) { 
        Mouse.move(0,0, -1);
      }
      else if (dy < 0) {
        Mouse.move(0,0, 1);
        }
        delay(100);
    }    

    delay(10);
  }
}
