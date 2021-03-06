//tracker config
#include "tracker.h"

#define debug_port SerialUSB
#include "debug.h"

//External libraries
#include <TinyGPS.h>
#include <DueFlashStorage.h>

//optimization
#define dtostrf(val, width, prec, sout) (void) sprintf(sout, "%" #width "." #prec "f", (double)(val))

bool debug_enable = true; // runtime flag to disable debug console

// Variables will change:
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long watchdogMillis = 0;        // will store last time modem watchdog was reset
int SEND_DATA = 1;

long time_start, time_stop, time_diff;             //count execution time to trigger interval
int interval_count = 0;         //current interval count (increased on each data collection and reset after sending)
int sms_check_count = 0;        //counter for SMS check (increased on each cycle and reset after check)

char data_current[DATA_LIMIT];  //data collected in one go, max 2500 chars
int data_index = 0;             //current data index (where last data record stopped)
char time_char[24];             //time attached to every data line
char modem_reply[200];          //data received from modem, max 200 chars
uint32_t logindex = STORAGE_DATA_START;
bool save_config = false;       //flag to save config to flash
bool power_reboot = false;      //flag to reboot everything (used after new settings have been saved)
bool power_cutoff = false;      //flag to cut-off power to avoid deep-discharge (no more operational afterwards)

char lat_current[15];
char lon_current[15];

unsigned long last_time_gps = -1, last_date_gps = 0, last_fix_gps = 0;

int engineRunning = -1;
unsigned long engineRunningTime = 0;
unsigned long engine_start;

DueFlashStorage dueFlashStorage;

int gsm_send_failures = 0;
int gsm_reply_failures = 0;

//settings structure
struct Settings {
  byte version;
  bool has_run;
  char apn[64];
  char user[20];
  char pwd[20];
  long interval;          //how often to collect data (milli sec, 600000 - 10 mins)
  char key[12];           //key for connection, will be sent with every data transmission
  char sim_pin[5];        //PIN for SIM card
  char sms_key[12];       //password for SMS commands
  char imei[20];          //IMEI number
  char iccid[25];         //ICCID number
  byte alarm_on;
  char alarm_phone[20];   //alarm phone number
  byte debug;             //flag to enable/disable debug console (USB)
};

Settings config;

//define serial ports
#define gps_port Serial1
#define gsm_port Serial2

void setup() {
  //common hardware initialization
  device_init();
  battery_init();

  // Load settings
  settings_load();

  //setting debug serial port
  DEBUG_FUNCTION_CALL();

  //initialize GSM and GPS hardware
  gsm_init();
  gps_init();

  //blink software start
  blink_start();

  // read/modify settings
  int first_boot = settings_load();

  //get current log index
#if STORAGE
  storage_get_index();
#endif

  //GPS setup
  gps_setup();

  //GSM setup
  gsm_setup();

  // reply to Alarm SMS command
  if (config.alarm_on) {
    sms_send_msg("Alarm Activated", config.alarm_phone);
  }
  
  // make sure we start with empty data
  data_reset();

  // with a new SIM delete APN config and require a new one
  if (config.iccid[0] && settings_compare(offsetof(Settings, iccid), strlen(config.iccid))) {
    DEBUG_PRINTLN(F("New SIM detected!"));
    // try with current APN first
    gsm_set_apn();
    // auto scanning of APN configuration
    int ap = gsm_scan_known_apn();
    if (ap) {
      settings_save(); // found good APN, save it as default
    }
  }

  // do not proceed until APN configured
  while (config.apn[0] == 0)
  {
    // ensure SMS command check at power on or reset
    sms_check();

    if (save_config == 1) {
      //config should be saved
      settings_save();
    }

    if (power_cutoff) // apply cut-off
      kill_power();
  }

  //set GSM APN
  gsm_set_apn();

#ifdef GSM_USE_NTP_SERVER
  // attempt clock update (over data connection)
  gsm_ntp_update();
#endif

  DEBUG_FUNCTION_PRINTLN("before loop");

  // ensure SMS command check at power on or reset
  sms_check();

  // apply runtime debug option (USB console) after setup
  if (config.debug == 1)
    usb_console_restore();
  else
    usb_console_disable();
}

void loop() {
  if (save_config == 1) {
    //config should be saved
    settings_save();
  }

  if (power_reboot == 1) {
    //reboot unit
    reboot();
    power_reboot = 0;
  }

  if (power_cutoff) {
    kill_power();
  }

  // Check if ignition is turned on
  int IGNT_STAT = digitalRead(PIN_S_DETECT);
  DEBUG_PRINT("Ignition status:");
  DEBUG_PRINTLN(IGNT_STAT);

  // detect transitions from engine on and off
  if (IGNT_STAT == 0) {
    if (engineRunning != 0) {
      // engine started
      engine_start = millis();
      engineRunning = 0;

      if (config.alarm_on == 1) {
        sms_send_msg("Ignition ON", config.alarm_phone);
      }
      // restore full speed for serial communication
      cpu_full_speed();
      gsm_open();
      // restart sending data
      collect_all_data(IGNT_STAT);
      send_data();
    }
  } else {
    if (engineRunning != 1) {
      // engine stopped
      if (engineRunning == 0) {
        engineRunningTime += (millis() - engine_start);
        if (config.alarm_on == 1) {
          sms_send_msg("Ignition OFF", config.alarm_phone);
        }
        // force sending last data
        interval_count = 1;
        collect_all_data(IGNT_STAT);
        send_data();
      }
      engineRunning = 1;
      // save power when engine is off
      gsm_deactivate(); // ~20mA less
      gsm_close();
      cpu_slow_down(); // ~20mA less
    }
  }

  if (!SEND_DATA) {
    time_stop = millis();

    //signed difference is good if less than MAX_LONG
    time_diff = time_stop - time_start;
    time_diff = config.interval - time_diff;

    DEBUG_PRINT("Sleeping for:");
    DEBUG_PRINT(time_diff);
    DEBUG_PRINTLN("ms");

    if (time_diff < 1000) {
      status_delay(1000);
    } else {
      while (time_diff > 1000) {
        status_delay(1000);
        time_diff -= 1000;
        // break earlier if ignition status changed
        if (IGNT_STAT != digitalRead(PIN_S_DETECT))
          time_diff = 0;
      }
      status_delay(time_diff);
    }
  } else {
    status_delay(1000);
  }

  //start counting time
  time_start = millis();

  if (IGNT_STAT == 0) {
    DEBUG_PRINTLN("Ignition is ON!");

    //collecting GPS data
    collect_all_data(IGNT_STAT);
    send_data();

#if SMS_CHECK_INTERVAL_ENGINE_RUNNING > 0
    // perform SMS check
    if (++sms_check_count >= SMS_CHECK_INTERVAL_ENGINE_RUNNING) {
      sms_check_count = 0;

      // facilitate SMS exchange by turning off data session
      gsm_deactivate();

      sms_check();
    }
#endif
  } else {
    DEBUG_PRINTLN("Ignition is OFF!");
    // Insert here only code that should be processed when Ignition is OFF

#if SMS_CHECK_INTERVAL_COUNT > 0
    // perform SMS check
    if (++sms_check_count >= SMS_CHECK_INTERVAL_COUNT) {
      sms_check_count = 0;

      // restore full speed for serial communication
      cpu_full_speed();
      gsm_open();

      sms_check();

      // back to power saving
      gsm_close();
      cpu_slow_down();
    }
#endif
  }

  status_led();
}
