#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdarg.h>

LiquidCrystal_I2C lcd(0x27,16,2);

const int PIN_RESET = 7;
const int PIN_P0    = 12;
const int PIN_P1    = 13;

const char *PLAYER_P0_NAMES[2] = {"Dad  ", "Blue"};
const char *PLAYER_P1_NAMES[2] = {"Jacob", "Red "};

const int MAX_SCORE = 10;
const unsigned long DEBOUNCE_THRESHOLD = 30;

struct Button {
  int pin;
  bool state;              // debounced level (HIGH == pressed with pulldown wiring)
  bool stateChanged;       // latched on a debounced transition
  unsigned long lastChangeMs; // time of last raw flip
  bool lastRaw;            // last instantaneous read
};

Button buttons[3] = {
  {PIN_RESET, false, false, 0, false},
  {PIN_P0,    false, false, 0, false},
  {PIN_P1,    false, false, 0, false}
};

int  scores[2] = {0, 0};
bool gameOver  = false;
int playerNameIndex = 0;

void lcdPrintf(const char *fmt, ...) {
  char buf[32];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd.print(buf);
}

void print_scores() {
  lcd.setCursor(0,0);
  lcdPrintf("%s %3d", PLAYER_P0_NAMES[playerNameIndex], scores[0]);
  lcd.setCursor(0,1);
  lcdPrintf("%s %3d",  PLAYER_P1_NAMES[playerNameIndex], scores[1]);
}

void print_winner() {
  lcd.clear();
  const char *winner = (scores[0] >= MAX_SCORE) ? PLAYER_P0_NAMES[playerNameIndex] : PLAYER_P1_NAMES[playerNameIndex];
  lcdPrintf("%s is the", winner);
  lcd.setCursor(0,1);
  lcdPrintf("winner!!!");
}

void print_invalid_input_error() {
  lcd.clear();
  lcd.print(F("Invalid Input"));
  delay(1000);
  lcd.clear();
  print_scores();
}

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(PIN_RESET, INPUT);
  pinMode(PIN_P0,    INPUT);
  pinMode(PIN_P1,    INPUT);

  // Initialize debouncer state from hardware
  unsigned long now = millis();
  for (int i = 0; i < 3; ++i) {
    bool raw = (digitalRead(buttons[i].pin) != 0);
    buttons[i].state = raw;
    buttons[i].lastRaw = raw;
    buttons[i].stateChanged = false;
    buttons[i].lastChangeMs = now;
  }

  print_scores();
}

int count_pressed_buttons() {
  return (buttons[0].state ? 1:0) + (buttons[1].state ? 1:0) + (buttons[2].state ? 1:0);
}


void debounce_read(Button &b) {
  bool raw = (digitalRead(b.pin) != 0);
  unsigned long t = millis();

  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.lastChangeMs = t;   
  }

  if ((t - b.lastChangeMs) >= DEBOUNCE_THRESHOLD) {
    if (raw != b.state) {
      b.state = raw;
      b.stateChanged = true;  // edge (debounced)
    }
  }
}

void reset_scores() {
  if (scores[0] == 0 && scores[1] == 0) // overload reset button and use it to change player names
    playerNameIndex = (playerNameIndex + 1) % 2;

  scores[0] = scores[1] = 0;
  gameOver = false;
  lcd.clear();
  print_scores();
}

void loop() {
  // Update all debouncers
  for (int i = 0; i < 3; ++i) {
    buttons[i].stateChanged = false;  
    debounce_read(buttons[i]);
  }

  // only allow 1 button to be pressed at a time
  if (count_pressed_buttons() > 1) {
    print_invalid_input_error();
    return;
  }

  // Handle debounced press edges only
  for (int i = 0; i < 3; ++i) {
    if (!buttons[i].stateChanged) continue;
    if (!buttons[i].state) continue;

    if (i == 0) {           // RESET button
      reset_scores();
      return;               // avoid scores[i-1] paths
    }

    if (gameOver) return;

    int p = i - 1;          // 0 for P0, 1 for P1
    if (scores[p] < MAX_SCORE) {
      scores[p]++;
      print_scores();
    }
    if (scores[p] >= MAX_SCORE) {
      gameOver = true;
      print_winner();
      return;
    }
  }
}
