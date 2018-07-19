#ifndef userInterface_c
#define userInterface_c

// Includes
#include "timeClock.h"
#include "userInterface.h"


// States
#define UI_INIT_S 0
#define UI_IDLE_S 1
#define UI_DRIVE_ALARM_S 2
#define UI_SET_TIME_SECONDS_S 3
#define UI_SET_TIME_MINUTES_S 4
#define UI_SET_TIME_HOURS_S 5
#define UI_SET_TIME_AMPM_S 6
#define UI_SET_BUTTON_RELEASE_S 7
#define UI_SET_FLASH_S 8

// Button Aliases
#define UI_B0 (BH_getAnIO('B',0))
#define UI_B1 (BH_getAnIO('B',1))
#define UI_B2 (BH_getAnIO('B',2))
#define UI_B3 (BH_getAnIO('B',3))
#define UI_B4 (BH_getAnIO('B',4))
#define UI_B5 (BH_getAnIO('B',5))
#define UI_SW0 (BH_getAnIO('S',0))

// Counters
uint16_t ui_updateDisp_ctr = 0;
uint16_t ui_flashDisp_ctr = 0;

// State Variables
uint8_t ui_currentState = UI_INIT_S;
uint8_t ui_flashLastState = UI_SET_TIME_SECONDS_S;
uint8_t ui_buttonRelease_NextState = UI_INIT_S;
uint8_t ui_buttonRelease_ButtonNum = 0;

// The time piece to set
extern timePiece TIME_CLK;
timePiece* settingTimePiece = &TIME_CLK;

char alarmTime[DISP_STRING_LEN] = {'0', '6', '.', '3', '0', '.', '0', '0', ' ', 'A'};
char timeOnDisplay[DISP_STRING_LEN] = {0};
//////////////////


timePiece TIME_CLK;
timePiece ALARM_CLK;


// Saves the display time as timeOnDisplay
void ui_storeDispTime(char* disp_time)
{
  for (uint8_t m = 0; m < DISP_STRING_LEN; m++)
    timeOnDisplay[m] = disp_time[m];
}


// A debug print
void ui_printDisp(char* dispString)
{
  for (uint8_t m = 0; m < DISP_STRING_LEN; m++)
  {
    Serial.print(dispString[m]);
  }
  //Serial.println(" ");
}


// Updates the display
void ui_updateDisplay(timePiece* TmPc)
{
  // Get the current time
  char timeString[TIMESTRINGLENGTH] = {0};
  timeClock_getTime(TmPc,timeString);

  // Extract characters
  char dispString[DISP_STRING_LEN] = {0};
  dispString[0] = timeString[0];
  dispString[1] = timeString[1];
  dispString[2] = '.';
  dispString[3] = timeString[3];
  dispString[4] = timeString[4];
  dispString[5] = '.';
  dispString[6] = timeString[6];
  dispString[7] = timeString[7];
  dispString[8] = ' ';

  // Check Format from switch
  if (twelveHourClk_flag) // High =  12 hour
    dispString[9] = timeString[13];
  else // Low = 24 hour
    dispString[9] = ' ';


  // Print Text to Seven Segment
  MX_disp_string(dispString, DISP_STRING_LEN);
  // Store Time
  ui_storeDispTime(dispString);


  // DEBUG, Print time
  //ui_printDisp(dispString);
}



// Checks if the alarm should go
bool ui_checkForAlarmTrigger()
{
  // Check if alarm is set
  if (BH_getAnIO('S', 0) == 0)
    return false;
    
  // Compare the TM and AL object
  
  if(TIME_CLK.hours != ALARM_CLK.hours)
    return false;
  if(TIME_CLK.minutes != ALARM_CLK.minutes)
    return false;
  if(TIME_CLK.seconds != ALARM_CLK.seconds)
    return false;
  // Times match (not milliseconds)
  Serial.println("Alarm Match!");
  return true;
}


// Tick function for the user interface
void ui_tick()
{
  // State Machine
  switch (ui_currentState)
  {

    ///////////////////////
    // Intialize everything
    case (UI_INIT_S):
      // Action //
      timeClock_init(&TIME_CLK,INTERUPTS_PER_SECOND, twelveHourClk_flag, START_SEC, START_MIN, START_HOURS);
      timeClock_init(&ALARM_CLK,INTERUPTS_PER_SECOND, twelveHourClk_flag, START_SEC+5, START_MIN, START_HOURS);
      ui_updateDisp_ctr = 0;
      tickClock_flag = true;
      // Pre-seed at half duty.
      ui_flashDisp_ctr = (INTERUPTS_PER_SECOND/2);
      // Advance //
      ui_currentState = UI_IDLE_S;
      break;

    ////////////////
    // Waiting state
    case (UI_IDLE_S):
      // Action //
      ui_updateDisp_ctr++;

      // Advance //

      // Main Time Set Mode
      if (UI_B5)
      {
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 5;
        // Do hours first
        ui_buttonRelease_NextState = UI_SET_TIME_HOURS_S;
        // Stop ticking clock
        tickClock_flag = false;
        //Serial.println("Moving to set Hours");
        // Setting the main time piece (TM) 
        settingTimePiece = &TIME_CLK;
        break;
      }

      // Alarm Time Set Mode
      if (UI_B4)
      {
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 4;
        // Do hours first
        ui_buttonRelease_NextState = UI_SET_TIME_HOURS_S;
        // Keep ticking clock
        tickClock_flag = true;
        //Serial.println("Moving to set Hours");
        // Setting the main time piece (TM) 
        settingTimePiece = &ALARM_CLK;
        break;
      }

      
      // Check for alarm
      if (ui_checkForAlarmTrigger())
      {
        ui_currentState = UI_DRIVE_ALARM_S;
        soundAlarm_flag = true;
        break;
      }

      
      // Update Display after so many ticks
      if (ui_updateDisp_ctr >= DISP_UPDATE_TICKS)
      {
        ui_currentState = UI_IDLE_S;
        ui_updateDisplay(&TIME_CLK);
        ui_updateDisp_ctr = 0;
        break;
      }
      break;

    ////////////////
    // Drive Alarm state
    case (UI_DRIVE_ALARM_S):
      // Action //

      // Advance //
      // Check if alarm switch is on
      if (BH_getAnIO('S', 0))
      {
        ui_currentState = UI_DRIVE_ALARM_S;
        soundAlarm_flag = true;
      }
      else // User turned off
      {
        ui_currentState = UI_IDLE_S;
        soundAlarm_flag = false;
      }
      // Update Alarm
      BZ_alarmSong(soundAlarm_flag);
      break;

    //////////////////////
    // Wait for release
    case (UI_SET_BUTTON_RELEASE_S):
      if (BH_getAnIO('B',ui_buttonRelease_ButtonNum))
      {
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        MX_dispTest(true);
      }
      else
      {
        ui_currentState = ui_buttonRelease_NextState;
        MX_dispTest(false);
      }
      break;

    //////////////////////
    // Set seconds
    case (UI_SET_TIME_SECONDS_S):
      // Action //
      //Serial.println("Inside set seconds");

      // Increase
      if (UI_B2)
      {
        timeClock_tickFWD(settingTimePiece,1000,1,0,0);
        //Serial.println("seconds++");
      }
      // Decrease
      else if (UI_B1)
      {
        timeClock_tickREV(settingTimePiece,1000,1,0,0);
        //Serial.println("seconds--");
      }


      // Advance //

      // Exit Set Mode Time Mode
      if ((UI_B5)&&(settingTimePiece==&TIME_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 5;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      // Exit Set Mode Alarm Mode
      if ((UI_B4)&&(settingTimePiece==&ALARM_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 4;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      
      // Left
      if (UI_B3)
      {
        //Serial.println("Going to Minutes");
        ui_buttonRelease_ButtonNum = 3;
        ui_buttonRelease_NextState = UI_SET_TIME_MINUTES_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }
      // Right
      if (UI_B0)
      {
        //Serial.println("Going to AMPM");
        ui_buttonRelease_ButtonNum = 0;
        ui_buttonRelease_NextState = UI_SET_TIME_AMPM_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }


      // Flash if nothing else
      ui_flashLastState = UI_SET_TIME_SECONDS_S;
      ui_currentState = UI_SET_FLASH_S;
      break;
      
    //////////////////////
    // Set minutes
    case (UI_SET_TIME_MINUTES_S):
      // Action //

      //Serial.println("Inside set Minutes");

      // Increase
      if (UI_B2)
      {
        timeClock_tickFWD(settingTimePiece,1000,60,1,0);
        //Serial.println("Minutes++");
      }
      // Decrease
      else if (UI_B1)
      {
        timeClock_tickREV(settingTimePiece,1000,60,1,0);
        //Serial.println("Minutes--");
      }

      // Advance //

      // Exit Set Mode Time Mode
      if ((UI_B5)&&(settingTimePiece==&TIME_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 5;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      // Exit Set Mode Alarm Mode
      if ((UI_B4)&&(settingTimePiece==&ALARM_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 4;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      
      // Left
      if (UI_B3)
      {
        //Serial.println("Going to Hours");
        ui_buttonRelease_ButtonNum = 3;
        ui_buttonRelease_NextState = UI_SET_TIME_HOURS_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }
      // Right
      if (UI_B0)
      {
        //Serial.println("Going to Seconds");
        ui_buttonRelease_ButtonNum = 0;
        ui_buttonRelease_NextState = UI_SET_TIME_SECONDS_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }


      // Flash if nothing else
      ui_flashLastState = UI_SET_TIME_MINUTES_S;
      ui_currentState = UI_SET_FLASH_S;
      break;


    //////////////////////
    // Set Hours
    case (UI_SET_TIME_HOURS_S):
      // Action //
      //Serial.println("Inside set Hours");

      // Increase
      if (UI_B2)
      {
        timeClock_tickFWD(settingTimePiece,1000,60,60,1);
        //Serial.println("Hours++");
      }
      // Decrease
      else if (UI_B1)
      {
        timeClock_tickREV(settingTimePiece,1000,60,60,1);
        //Serial.println("Hours--");
      }

      // Advance //
      // Exit Set Mode Time Mode
      if ((UI_B5)&&(settingTimePiece==&TIME_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 5;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      // Exit Set Mode Alarm Mode
      if ((UI_B4)&&(settingTimePiece==&ALARM_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 4;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      
      // Left
      if (UI_B3)
      {
        //Serial.println("Going to AMPM");
        ui_buttonRelease_ButtonNum = 3;
        ui_buttonRelease_NextState = UI_SET_TIME_AMPM_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }
      // Right
      if (UI_B0)
      {
        //Serial.println("Going to Minutes");
        ui_buttonRelease_ButtonNum = 0;
        ui_buttonRelease_NextState = UI_SET_TIME_MINUTES_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }


      // Flash if nothing else
      ui_flashLastState = UI_SET_TIME_HOURS_S;
      ui_currentState = UI_SET_FLASH_S;
      break;

    //////////////////////
    // Set AM or PM
    case (UI_SET_TIME_AMPM_S):
      // Action //
      //Serial.println("Inside set AMPM");

      // Increase
      if (UI_B2)
      {
        timeClock_tickFWD(settingTimePiece,1000,60,60,12);
        //Serial.println("AMPM++");
      }
      // Decrease
      else if (UI_B1)
      {
        timeClock_tickREV(settingTimePiece,1000,60,60,12);
        //Serial.println("AMPM--");
      }

      // Advance //

      // Exit Set Mode Time Mode
      if ((UI_B5)&&(settingTimePiece==&TIME_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 5;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      // Exit Set Mode Alarm Mode
      if ((UI_B4)&&(settingTimePiece==&ALARM_CLK))
      {
        //Serial.println("Leaving to Idle");
        // Set up the button release parameters
        ui_buttonRelease_ButtonNum = 4;
        ui_buttonRelease_NextState = UI_IDLE_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        // Start ticking clock
        tickClock_flag = true;
        break;
      }
      
      // Left
      if (UI_B3)
      {
        //Serial.println("Going to Seconds");
        ui_buttonRelease_ButtonNum = 3;
        ui_buttonRelease_NextState = UI_SET_TIME_SECONDS_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }
      // Right
      if (UI_B0)
      {
        //Serial.println("Going to Hours");
        ui_buttonRelease_ButtonNum = 0;
        ui_buttonRelease_NextState = UI_SET_TIME_HOURS_S;
        ui_currentState = UI_SET_BUTTON_RELEASE_S;
        break;
      }


      // Flash if nothing else
      ui_flashLastState = UI_SET_TIME_AMPM_S;
      ui_currentState = UI_SET_FLASH_S;
      break;


    //////////////////////
    // MAKE FLASHING
    case (UI_SET_FLASH_S):
      // Action //
      ui_flashDisp_ctr= (ui_flashDisp_ctr+1)%int(INTERUPTS_PER_SECOND);
      if(ui_flashDisp_ctr<DISP_FLASH_TICKS)
        MX_writeBLANK();
      else
        ui_updateDisplay(settingTimePiece);
      // Advance // 
      ui_currentState = ui_flashLastState;
      break;
      
    ///////
    default:
      ui_currentState = UI_INIT_S;
      break;
  }
}




#endif