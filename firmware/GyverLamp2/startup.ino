void checkButton() {
#if (USE_BTN == 1)
  DEBUGLN(cfg.WiFimode ? "local mode" : "AP mode");
  if (btn.isHold()) {          // кнопка зажата
    ledController.clearLedData();
    byte count = 0;
    bool state = 0;

    while (btn.state()) {     // пока зажата кнопка
      fill_solid(leds, constrain(count, 0, 8), CRGB::Red);
      count++;
      if (count == 9) {               // на счёт 9 поднимаем яркость и флаг
        FastLED.setBrightness(120);
        state = 1;
      } else if (count == 16) {       // на счёт 16 опускаем флаг выходим
        state = 0;
        break;
      }
      ledController.ShowLeds();
      delay(300);
    }
    if (state) {
      DEBUGLN("change mode");
      cfg.WiFimode = !cfg.WiFimode;
      EEPROM.put(0, cfg);
      EEPROM.commit();
      delay(100);
      ESP.restart();
    }
  }
  FastLED.setBrightness(50);
  ledController.clearLedData();
  ledController.ShowLeds();
#endif
}

void checkGroup() {
  fill_solid(leds, cfg.group, (cfg.WiFimode) ? (CRGB::Blue) : (CRGB::Green));
  ledController.ShowLeds();
  uint32_t tmr = millis();
  bool flag = 0;
  while (millis() - tmr < 3000) {
#if (USE_BTN == 1)
    btn.tick();
    if (btn.isClick()) {
      if (++cfg.group > 10) cfg.group = 1;
      ledController.clearLedData();
      fill_solid(leds, cfg.group, (cfg.WiFimode) ? (CRGB::Blue) : (CRGB::Green));
      ledController.ShowLeds();
      flag = 1;
      tmr = millis();
    }
    if (btn.isHold()) {
      return;
    }
#endif
    yield();
  }
  if (flag) {
    EEPROM.put(0, cfg);
    EEPROM.commit();
  }
  DEBUG("group: ");
  DEBUGLN(cfg.group);
  DEBUG("role: ");
  DEBUGLN(cfg.role);
}
//ToDo
void startBacklight() {

  backlihgtController = FastLED.addLeds<STRIP_CHIP, BACKLIGHT_PIN, STRIP_COLOR>(backlight_leds, MAX_BACKLIGHT_LEDS); // clean memory if necessary (delete the controllers)!!!

  backlight_leds[0] = CRGB::Aqua;
  backlight_leds[1] = CRGB::Aqua;
  backlight_leds[2] = CRGB::Aqua;
  backlight_leds[3] = CRGB::DarkViolet;
  backlight_leds[4] = CRGB::DarkViolet;
  backlight_leds[5] = CRGB::DarkViolet;
  //???What is that???
  //FastLED.addLeds<STRIP_CHIP, BACKLIGHT_PIN, STRIP_COLOR>(backlight_leds, MAX_BACKLIGHT_LEDS).setCorrection(TypicalLEDStrip);
  //???Is it correct???
  FastLED.setMaxPowerInVoltsAndMilliamps(STRIP_VOLT, 240);//How to set voltage for LedController&   
  backlihgtController.showLeds(50);
}

void startStrip() {
	
  ledController = FastLED.addLeds<STRIP_CHIP, STRIP_PIN, STRIP_COLOR>(leds, MAX_LEDS);
  //FastLED.addLeds<STRIP_CHIP, STRIP_PIN, STRIP_COLOR>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);  
  FastLED.setMaxPowerInVoltsAndMilliamps(STRIP_VOLT, 60*40/*mA per led*/);
  FastLED.setBrightness(50);
  ledController.showLeds();
}

void showRGB() {
  leds[0] = CRGB::Red;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Blue;
  ledController.ShowLeds();
  ledController.clearLedData();
  delay(1500);
}

void startWiFi() {
  if (!cfg.WiFimode) setupAP();   // режим точки доступа
  else setupLocal();              // подключаемся к точке

  restartUDP();
  ledController.clearLedData();
  ledController.ShowLeds();
}

void setupAP() {
  blink16(CRGB::Yellow);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAP(AP_NameChar, WiFiPassword);
  server.begin();
  DEBUGLN("Setting AP Mode");
  DEBUG("AP IP: ");
  DEBUGLN(WiFi.softAPIP());
  delay(500);
}

void setupLocal() {
  if (cfg.ssid[0] == NULL && cfg.pass[0] == NULL) {
    DEBUGLN("WiFi not configured");
    setupAP();
  } else {
    DEBUGLN("Connecting to AP...");
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(100);
    uint32_t tmr = millis();
    bool connect = false;
    int8_t count = 0, dir = 1;
    byte failCount = 0;
    while (1) {
      WiFi.begin(cfg.ssid, cfg.pass);
      while (millis() - tmr < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          connect = true;
          break;
        }
        ledController.clearLedData();
        leds[count] = CRGB::Yellow;
        ledController.ShowLeds();
        count += dir;
        if (count >= 15 || count <= 0) dir *= -1;
        delay(50);
      }
      if (connect) {
        connTmr.stop();
        blink16(CRGB::Green);
        server.begin();
        DEBUG("Connected! Local IP: ");
        DEBUGLN(WiFi.localIP());
        delay(500);
        return;
      } else {
        DEBUGLN("Failed!");
        blink16(CRGB::Red);
        failCount++;
        tmr = millis();
        if (failCount >= 3) {
          connTmr.restart();    // попробуем позже
          setupAP();
          return;
        }
      }
    }
  }
}

void checkUpdate() {
  if (cfg.update) {   // было ОТА обновление
    if (cfg.version != GL_VERSION) {
      cfg.version = GL_VERSION;
      blink16(CRGB::Cyan);
      DEBUG("Update to");
      DEBUGLN(GL_VERSION);
    } else {
      blink16(CRGB::Blue);
      DEBUGLN("Update to current");
    }
    cfg.update = 0;
    EE_updCfg();
  } else {
    if (cfg.version != GL_VERSION) {
      cfg.version = GL_VERSION;
      blink16(CRGB::Cyan);
      DEBUG("Update to");
      DEBUGLN(GL_VERSION);
    }
  }
}

void tryReconnect() {
  if (connTmr.isReady()) {
    DEBUGLN("Reconnect");
    startWiFi();
  }
}

void misc() {
  memset(matrixValue, 0, sizeof(matrixValue));
  char GLkey[] = GL_KEY;
  portNum = 17;
  for (byte i = 0; i < strlen(GLkey); i++) portNum *= GLkey[i];
  portNum %= 15000;
  portNum += 50000;
}
