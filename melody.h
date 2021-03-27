#include "pithces.h"

#ifndef MELODY_H
#define MELODY_H

#define BUZZER_PIN       13
#define REST             0

static int melodyUp[] = {
  NOTE_C4, 2, NOTE_D5, 2,
};
static int melodyUpLength = sizeof(melodyUp) / sizeof(melodyUp[0]);


static int melodyDown[] = {
  NOTE_D5, 2, NOTE_C4, 2,
};
static int melodyDownLength = sizeof(melodyDown) / sizeof(melodyDown[0]);

static int melodyBeep[] = {
  NOTE_C5, 2, REST, 2,
};
static int melodyBeepLength = sizeof(melodyBeep) / sizeof(melodyBeep[0]);

static int melodyAlarm[] = {
  NOTE_C7, 4, REST, 4,
};
static int melodyAlarmLength = sizeof(melodyAlarm) / sizeof(melodyAlarm[0]);

void playMelody (int melody[], int length);

#endif // MELODY_H