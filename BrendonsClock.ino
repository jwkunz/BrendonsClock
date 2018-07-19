#include "TimerOne.h"
#include "timeClock.h"
#include "userInterface.h"
#include "buzzerDriver.h"

// Determined empiracally
#define INTERUPT_TUNE_FACTOR (+104)
// +141 is slow
// +0 is fast
// +50 is fast
// +100 is fast
// +104 is
// +108 is slow (barely)
// +115 is slow
// +128 is slow

void setup() {
  Serial.begin(9600);
  Serial.println("Booted");
  BH_initAll();
  MX_init();
  BZ_init();
  Serial.println("Starting Interupts");
  Timer1.initialize(INTERUPT_PERIOD_US+INTERUPT_TUNE_FACTOR);
  Timer1.attachInterrupt(mainISR);
}

uint64_t tickCount = 0;
uint32_t lastTime = 0;
uint32_t elapsedTime = 0;
uint32_t timeMark = 0;


void printElapsedTime()
{
  timeMark = micros();
  elapsedTime = timeMark-lastTime;
  lastTime = timeMark;
  Serial.println(elapsedTime);
}

void loop() {
}

extern timePiece TIME_CLK;

void mainISR()
{
  //printElapsedTime();
  // Counter
  tickCount++;
  // Tick Clock if enabled
  if (ui_getTickStatus())
    timeClock_tickFWD(timeClock_getClock(),INTERUPT_PERIOD_US/1000,1,1,1);
  // Tick the User Interface
  ui_tick();
  printTime(timeClock_getClock()); 
}


void printTime(timePiece* TmPc)
{
  char dispString[TIMESTRINGLENGTH] = {0};
  timeClock_getTime(TmPc,dispString);
  for(uint8_t m = 0; m < TIMESTRINGLENGTH; m++)
  {
    Serial.print(dispString[m]);
  }
  Serial.println(" ");
}

