#include "config.h"

const auto WAKEUP_FUNCTION = [](void){/* Do nothing, just wake up */};
const int  INTERRUPT_PIN   = 2;

void setup() {
  Serial.begin(9600);

  // attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), WAKEUP_FUNCTION, LOW);
}

void loop() {
  //
}
