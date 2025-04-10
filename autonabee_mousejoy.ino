#include "HID-Project.h"
#include "EEPROM.h"

#define SwitchScroll 0
#define SwitchShift 1
#define DEFAULT_DEADZONE 50.0
#define DEFAULT_SPEED 8.0
#define DEFAULT_OFFSET 1024/2
#define COMMAND_CALIBRATE "CommandCalibrate"
#define ADRESS_DX_OFS 0
#define ADRESS_DY_OFS 2
// expect SET SPEED ?
#define ADRESS_SPEED  4
// expect SET DEADZONE ?
#define ADRESS_DEADZONE 6

int all_signals_length = 4;
int all_signals[] = { A2, A3, A4, A5 };

int button_lengths = 2;
int button_signals[] = {A2, A3};
bool button_pressed[] = {false, false};
int buttons_id[] = {MOUSE_LEFT, MOUSE_RIGHT};

int switches_lengths = 2;
int switch_scroll_idx = 0;
int switches_signals[] = {A4, A5};
bool switches_toggled[] = {false, false};
int switches_id[] = {
  SwitchScroll,
  SwitchShift
};

int dx_ofs = 0;
int dy_ofs = 0;
int deadzone = 0;
int speed = 0;

void load_values_from_EEPROM() {
  dx_ofs = readIntFromEEPROM(ADRESS_DX_OFS);
  dy_ofs = readIntFromEEPROM(ADRESS_DY_OFS);
  speed = readIntFromEEPROM(ADRESS_SPEED);
  deadzone = readIntFromEEPROM(ADRESS_DEADZONE);
}

void setup() {
  load_values_from_EEPROM();
  while (!Serial) {
    print_tick("X offset", String(dx_ofs));
    print_tick("Y offset", String(dy_ofs));
    print_tick("Speed", String(speed));
    print_tick("Deadzone", String(deadzone));
  }
  // we init all the inputs.
  for (int i = 0; i < all_signals_length; i++) {
    pinMode(all_signals[i], INPUT_PULLUP); // pas besoin de resistances pour les switchs.
  }
  Serial.begin(9600);
  Serial.println("Arduino joy mouse starts!");

  // Sends a clean report to the host. This is important on any Arduino type.
  Mouse.begin();
}

void print_tick(String name, String s) {
      Serial.print(name);
      Serial.print(": '");
      Serial.print(s);
      Serial.println("'");
}

void handle_serial_communication() {
  while (Serial.available() > 0) {
    // todo expect set get calibrate
    String command = Serial.readString();
    Serial.print("received: ");
    Serial.println(command);
    
    if (command.indexOf(COMMAND_CALIBRATE) != -1) {
        writeIntToEEPROM(ADRESS_DX_OFS, analogRead(A1));        
        writeIntToEEPROM(ADRESS_DY_OFS, analogRead(A0));
        Serial.print("Calibrated with ");
        Serial.print(analogRead(A1));
        Serial.print("-");
        Serial.println(analogRead(A0));
    }
    else if (command.indexOf("DEFAULTS") != -1) {
      // todo back to defaults.
    }
    else {
      int space = command.indexOf(" ");
      if (space == -1) {
          Serial.print("Command invalid: ");
          Serial.println(command);
      }
      String set_part = command.substring(0, space);
      String rest = command.substring(space + 1); // we skip the space itself
      print_tick("first word", set_part);
      if (!set_part.equals("SET")) {
          Serial.print("EXPECTED SET, FOUND: ");
          Serial.println(set_part);
          continue;
      }

      space = rest.indexOf(" ");
      if (space == -1) {
          Serial.println("Expected VARIABLE VALUE");
          Serial.print("received: ");
          Serial.println(command);
      }
      
      String variable = rest.substring(0, space);
      String value = rest.substring(space + 1);
      //print_tick("variable", variable);
      //print_tick("value", value);

      int parsed_value = value.toInt();
      if (parsed_value == 0) {
        Serial.print("Invalid Value: ");
        Serial.println(value);
        Serial.println("Expected an integer > 0");
        continue;
      }
      
      if (variable.equals("SPEED")) {
        writeIntToEEPROM(ADRESS_SPEED, parsed_value);
        Serial.print("Set speed to ");
        Serial.println(parsed_value);
        load_values_from_EEPROM();
        
      }
      else if (variable.equals("DEADZONE")) {
        writeIntToEEPROM(ADRESS_DEADZONE, parsed_value);
        Serial.print("Set deadzone to ");
        Serial.println(parsed_value);
        load_values_from_EEPROM();
      }
      else {
        print_tick("Expected SPEED or DEADZONE, received: ", value);
      }
    }
  }
}


void loop() {  
  
  handle_serial_communication();
  
  for (int i = 0; i < button_lengths; i++) {
    if (not digitalRead(button_signals[i]) and not button_pressed[i]) {
      button_pressed[i] = true;
      Mouse.press(buttons_id[i]);
      Serial.print("button press: ");
      Serial.print(i);
      Serial.print("\n");
    }
    else if (digitalRead(button_signals[i]) and button_pressed[i]) {
      button_pressed[i] = false;
      
      Mouse.release(buttons_id[i]);
      Serial.print("release: ");
      Serial.print(i);
      Serial.print("\n");
    }
  }

  for (int i = 0; i < switches_lengths; i++) {
    if (!digitalRead(switches_signals[i])) {
      switches_toggled[i] = !switches_toggled[i];
      if (switches_signals[i] == SwitchShift) {
        // todo do something with keyboard
      }
      delay(50); // simple debouncing
    }
  }

  float dx = -(analogRead(A1) - dx_ofs) ;
  float dy = -(analogRead(A0) - dy_ofs) ;
  if (abs(dx) <= deadzone ) {dx = 0.0;}
  if (abs(dy) <= deadzone ) {dy = 0.0;}

  if (!switches_toggled[switch_scroll_idx])  {
    Mouse.move(
      signOf(dx) * log10(1 + abs(dx)/1024*9) * speed,
      signOf(dy) * log10(1+ abs(dy)/1024 * 9) * speed);
  }
  else { // scroll
    // todo replace with scroll constants.
    if (dy > 0) { 
      Mouse.move(0,0, -2);
    }
    else if (dy < 0) {
      Mouse.move(0,0, 2);
      }
      delay(100);
  }
  delay(10);
}

int signOf(int i) {
  if (i < 0) { return -1; }
  return +1;
}

void writeIntToEEPROM(int address, int number)
{ 
  byte byte1 = number >> 8;
  byte byte2 = number & 0xFF;
  
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);

  return (byte1 << 8) + byte2;
}
