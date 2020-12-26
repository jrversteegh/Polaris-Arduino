
void settings_load_defaults() {
  DEBUG_FUNCTION_PRINTLN("using settings defaults");
  config.version = CONFIG_VERSION;
  config.interval = INTERVAL;
  config.alarm_on =  DEFAULT_ALARM_ON;
  config.debug = DEBUG ? 1 : 0;

  strlcpy(config.key, KEY, sizeof(config.key));
  strlcpy(config.sms_key, SMS_KEY, sizeof(config.sms_key));
  strlcpy(config.sim_pin, SIM_PIN, sizeof(config.sim_pin));
  strlcpy(config.apn, DEFAULT_APN, sizeof(config.apn));
  strlcpy(config.user, DEFAULT_USER, sizeof(config.user));
  strlcpy(config.pwd, DEFAULT_PASS, sizeof(config.pwd));
  strlcpy(config.alarm_phone, DEFAULT_ALARM_SMS, sizeof(config.alarm_phone));
  save_config = 1;
}

int settings_load(int readonly) {
  //load all settings from EEPROM
  byte tmp;
  static int defaults = 0;

  DEBUG_FUNCTION_CALL();

  byte* b = dueFlashStorage.readAddress(STORAGE_CONFIG_MAIN); // byte array which is read from flash at adress
  memcpy(&config, b, sizeof(Settings)); // copy byte array to temporary struct


  if(!config.has_run || config.version != CONFIG_VERSION) {
    //first run was not set, this is first even run of the board use config from tracker.h
    DEBUG_FUNCTION_PRINTLN("first run...");
    settings_load_defaults();

    config.has_run = false;
    defaults = 1;
  }

  //setting defaults in case something is incorrect
  DEBUG_FUNCTION_PRINT("config.interval=");
  DEBUG_PRINTLN(config.interval);

  if((config.interval < 0) || (config.interval > 5184000)) {
    DEBUG_FUNCTION_PRINTLN("interval not found, setting default");
    config.interval = INTERVAL;
    save_config = 1;

    DEBUG_FUNCTION_PRINT("set config.interval=");
    DEBUG_PRINTLN(config.interval);
  }

  DEBUG_FUNCTION_PRINT("config.debug=");
  DEBUG_PRINTLN(config.debug);

  if((config.debug != 1) && (config.debug != 0)) {
    DEBUG_FUNCTION_PRINTLN("debug not found, setting default");
    config.debug = DEBUG ? 1 : 0;
    save_config = 1;

    DEBUG_FUNCTION_PRINT("set config.debug=");
    DEBUG_PRINTLN(config.debug);
  }

  tmp = config.key[0];
  if(tmp == 255) { //this check is not sufficient
    DEBUG_FUNCTION_PRINTLN("key not found, setting default");
    strlcpy(config.key, KEY, sizeof(config.key));
    save_config = 1;
  }

  tmp = config.sms_key[0];
  if(tmp == 255) { //this check is not sufficient
    DEBUG_FUNCTION_PRINTLN("SMS key not found, setting default");
    strlcpy(config.sms_key, SMS_KEY, sizeof(config.sms_key));
    save_config = 1;
  }

  tmp = config.sim_pin[0];
  if(tmp == 255) { //this check is not sufficient
    DEBUG_FUNCTION_PRINTLN("SIM pin not found, setting default");
    strlcpy(config.sim_pin, SIM_PIN, sizeof(config.sim_pin));
    save_config = 1;
  }

  tmp = config.apn[0];
  if(tmp == 255) {
    DEBUG_FUNCTION_PRINTLN("APN not set, setting default");
    strlcpy(config.apn, DEFAULT_APN, sizeof(config.apn));
    save_config = 1;
  }

  tmp = config.user[0];
  if(tmp == 255) {
    DEBUG_FUNCTION_PRINTLN("APN user not set, setting default");
    strlcpy(config.user, DEFAULT_USER, sizeof(config.user));
    save_config = 1;
  }

  tmp = config.pwd[0];
  if(tmp == 255) {
    DEBUG_FUNCTION_PRINTLN("APN password not set, setting default");
    strlcpy(config.pwd, DEFAULT_PASS, sizeof(config.pwd));
    save_config = 1;
  }

  tmp = config.alarm_phone[0];
  if(tmp == 255) {
    DEBUG_FUNCTION_PRINTLN("Alarm SMS number not set, setting default");
    strlcpy(config.alarm_phone, DEFAULT_ALARM_SMS, sizeof(config.alarm_phone));
    save_config = 1;
  }

  if (readonly)
    save_config = 0;
    
  if (save_config == 1)
    settings_save();
  return defaults;
}

int settings_load() {
  return settings_load(0);
}

void settings_save() {
  DEBUG_FUNCTION_CALL();

  //save all settings to flash
  dueFlashStorage.write(STORAGE_CONFIG_MAIN, (byte*)&config, sizeof(Settings));
}

int settings_compare(size_t offset, size_t len) {
  // compare part of the stored settings with the same part in volatile memory
  byte* b = dueFlashStorage.readAddress(STORAGE_CONFIG_MAIN);
  return len > 0 ? memcmp((byte*)&config + offset, b + offset, len) : -2;
}
