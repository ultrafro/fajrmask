//bismillah

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include "Adafruit_MAX1704X.h"

// If you're using the full breakout...
//Adafruit_IS31FL3731 ledmatrix = Adafruit_IS31FL3731();
// If you're using the FeatherWing version
Adafruit_IS31FL3731_Wing ledmatrix = Adafruit_IS31FL3731_Wing();

// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_MAX17048 lipo;

const unsigned long SLEEP_TIME = 45000;

// The lookup table to make the brightness changes be more visible
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

float p = 3.1415926;

const int menuSize = 12;
int selectedIndex = 0;
int editIndex = 0;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

GFXcanvas16 canvas16(240, 135); //16bit


int hours[menuSize] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

int minutes[menuSize] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

bool actives[menuSize] = {
  false,
false,
false,
false,
false,
false,
false,
false,
false,
false,
false,
};

bool debugShowLight = false;

unsigned long startTime = 0;  // Time when the device started
unsigned long lastDisplayUpdateTime = 0;
unsigned long lightTurnOnTime = 0;
unsigned long lastInteractionTime = 0;
int hourOffset = 0;          // Offset to add to calculated hours
int minuteOffset = 0;        // Offset to add to calculated minutes
bool isAsleep = false;
bool haveLightsEverBeenOn = false;

int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("ISSI swirl test");

  startTime = millis();  // Record the start time

  // Setup pin 6 as output
  pinMode(6, OUTPUT);

  // Disable TFT I2C power and NeoPixel power
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW);

  if (! ledmatrix.begin()) {
    Serial.println("IS31 not found");
    while (1);
  }
  Serial.println("IS31 found!");

  if (!lipo.begin()) {
    Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    while (1) delay(10);
  }

  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x")); 
  Serial.println(lipo.getChipID(), HEX);

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

   // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);


  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);

  canvas16.setTextWrap(false);

  updateDisplay();



}

void loop() {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  unsigned long currentTime = millis();

  // Blink LED on pin 6 every 500ms
  if (currentTime - lastBlinkTime >= 500) {
    ledState = !ledState;
    digitalWrite(6, ledState);
    lastBlinkTime = currentTime;
  }

  bool shouldUpdateDisplay = false;

  bool redrawSelected = false;
  unsigned long now = millis();

  // Calculate current time based on startup time and offsets
  unsigned long elapsedMillis = now - startTime;
  unsigned long totalSeconds = elapsedMillis / 1000;
  unsigned long totalMinutes = totalSeconds / 60;
  unsigned long totalHours = totalMinutes / 60;
  
  // Calculate current time with offsets
  currentSecond = totalSeconds % 60;
  currentMinute = (totalMinutes % 60 + minuteOffset) % 60;
  currentHour = (totalHours % 24 + hourOffset + (totalMinutes % 60 + minuteOffset) / 60) % 24;

  // Update the clock time (index 0) with current time
  if (selectedIndex == 0) {
    hours[0] = currentHour;
    minutes[0] = currentMinute;
  }

  bool upPressed = digitalRead(0) == LOW && now - lastButtonPress>debounceDelay;
  bool downPressed = digitalRead(1) == HIGH && now - lastButtonPress>debounceDelay;
  bool editPressed = digitalRead(2) == HIGH && now - lastButtonPress>debounceDelay;

  if(upPressed || downPressed || editPressed){
    lastButtonPress = now;
    lastInteractionTime = now;
  }

  HandleAlarm();

  bool shouldSleep = now - lastInteractionTime > SLEEP_TIME;
  if(shouldSleep && !isAsleep){
    isAsleep = true;
    GoToSleep();
    return;
  }

  if(!shouldSleep && isAsleep){
    isAsleep = false;
    WakeUp();
    return;
  }

  if(isAsleep){
    //sleep for 50ms
    delay(50);
    return;
  }

  if(editPressed){
    int editLength = 4;
    if(selectedIndex == 0){
      editLength = 3;
    }

    if(selectedIndex == 11){
      editLength = 2;
    }
    
    int lastEditIndex = editIndex;    
    editIndex = (editIndex + 1)%editLength;
    shouldUpdateDisplay = true;

    if(selectedIndex == menuSize-1 && lastEditIndex == 1 && editIndex == 0){
      ShutOffLights();
    }
  }

  if(now - lastDisplayUpdateTime > 1000){
    shouldUpdateDisplay = true;
    lastDisplayUpdateTime = now;
  }

  

  if(editIndex == 0){
    if(upPressed){
      selectedIndex = (selectedIndex - 1 + menuSize) % menuSize;
      shouldUpdateDisplay = true;
    }

    if(downPressed){
      selectedIndex = (selectedIndex + 1) % menuSize;
      shouldUpdateDisplay = true;
    }
  }

  if(editIndex == 1){
    if(upPressed){
      if (selectedIndex == 0) {
        // When editing clock time, update the offset
        hourOffset = (hourOffset + 1) % 24;
      } else {
        if(selectedIndex == menuSize-1){
          TriggerLight(true);

        }else{
          int newHour = hours[selectedIndex] + 1;
          newHour = newHour % 24;
          hours[selectedIndex] = newHour;
        }
      }
      shouldUpdateDisplay = true;
    }

    if(downPressed){
      if (selectedIndex == 0) {
        // When editing clock time, update the offset
        hourOffset = (hourOffset - 1 + 24) % 24;
      } else {
        if(selectedIndex == menuSize-1){
          TriggerLight(true);
        }else{
          int newHour = hours[selectedIndex] - 1;
          newHour = (newHour + 24) % 24;
          hours[selectedIndex] = newHour;
        }
      }
      shouldUpdateDisplay = true;
    }
  }

  if(editIndex == 2){
    if(upPressed){
      if (selectedIndex == 0) {
        // When editing clock time, update the offset
        minuteOffset = (minuteOffset + 1) % 60;
      } else {
        if(selectedIndex == menuSize-1){
          debugShowLight = !debugShowLight;
        }else{
          int newMinutes = minutes[selectedIndex] + 1;
          newMinutes = newMinutes % 60;
          minutes[selectedIndex] = newMinutes;
        }
      }
      shouldUpdateDisplay = true;
    }

    if(downPressed){
      if (selectedIndex == 0) {
        // When editing clock time, update the offset
        minuteOffset = (minuteOffset - 1 + 60) % 60;
      } else {
        if(selectedIndex == menuSize-1){
          debugShowLight = !debugShowLight;
        }else{
          int newMinutes = minutes[selectedIndex] - 1;
          newMinutes = (newMinutes + 60) % 60;
          minutes[selectedIndex] = newMinutes;
        }
      }
      shouldUpdateDisplay = true;
    }
  }

  if(editIndex == 3){
    if(upPressed){
      actives[selectedIndex] = !actives[selectedIndex];
      shouldUpdateDisplay = true;
    }

    if(downPressed){
      actives[selectedIndex] = !actives[selectedIndex];
      shouldUpdateDisplay = true;
    }
  }




  UpdateLights();
  

  if(shouldUpdateDisplay){
    updateDisplay();
  }




  

  delay(10);
}

void GoToSleep(){

  //turn off backlight
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, LOW);

  delay(50);

  //turn off the TFT / I2C power supply
  digitalWrite(TFT_I2C_POWER, LOW);

  ShutOffLights();
}

void WakeUp(){

    //turn on the TFT / I2C power supply
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(50);

  //turn on backlight
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);


}

void HandleAlarm(){
  //trigger light if the hour and minut match an active alarm
  bool anyLightTriggered = false;
  for(int i = 0; i < menuSize; i++){
    if(i == 0 || i == menuSize-1){
      continue;
    }

    if(actives[i]){
      if(hours[i] == currentHour && minutes[i] == currentMinute){
        TriggerLight(false);
      }
    }
  }
}


unsigned long lastAnimationTime = 0;
void TriggerLight(bool force){

  lastInteractionTime = millis();
  
  unsigned long now = millis();
  unsigned long timeSinceLastAnimation = now - lastAnimationTime;
  if(!haveLightsEverBeenOn || timeSinceLastAnimation > 60000 || force){
    lightTurnOnTime = millis();
    lastAnimationTime = now;
    haveLightsEverBeenOn = true;
  }

}


void UpdateLights(){
  if(!haveLightsEverBeenOn){
    return;
  }
  unsigned long now = millis();
  unsigned long timeSinceLightTurnOn = now - lightTurnOnTime;



  int lightDuration = 40000;
  int lightStartPeriod = 1000;
  int lightEndPeriod = 500;
  

  if(timeSinceLightTurnOn > lightDuration){
    ShutOffLights();
    return;
  }

  //make value a sine wave of low frequency for first 10 seconds
  // then make it a high frequency sine wave for the next 10 seconds

  float period = 1000;
  if(timeSinceLightTurnOn < lightDuration/2){
    //low frequency sine wave
    period = lightStartPeriod;
  }else{
    //high frequency sine wave
    period = lightEndPeriod;
  }

  float pi = 3.1415926;

  float value = 0.5 + 0.5 * sin(timeSinceLightTurnOn * 2 * pi / period);

  int lightValue = (int)(value * 255);

  for (uint8_t x=0; x<16; x++) {
    for (uint8_t y=0; y<9; y++) {
      ledmatrix.drawPixel(x, y, lightValue);
    }
  }

}

void ShutOffLights(){
  int i = 0;
  for (uint8_t x=0; x<16; x++) {
    for (uint8_t y=0; y<9; y++) {
      ledmatrix.drawPixel(x, y, 0);
    }
  }


}


void updateInitialDisplay(){
  //clear the screen
  canvas16.fillScreen(0xFD20);  // Desert sand color

  // Define border colors
  uint16_t borderBrown = 0x8200;  // Dark brown
  uint16_t borderGreen = 0x0400;  // Dark green

  // Draw a simple border pattern
  int borderWidth = 4;
  
  // Top border
  for(int i = 0; i < canvas16.width(); i += 8) {
    canvas16.fillRect(i, 0, 4, borderWidth, borderBrown);
    canvas16.fillRect(i + 4, 0, 4, borderWidth, borderGreen);
  }
  
  // Bottom border
  for(int i = 0; i < canvas16.width(); i += 8) {
    canvas16.fillRect(i, canvas16.height() - borderWidth, 4, borderWidth, borderBrown);
    canvas16.fillRect(i + 4, canvas16.height() - borderWidth, 4, borderWidth, borderGreen);
  }
  
  // Left border
  for(int i = 0; i < canvas16.height(); i += 8) {
    canvas16.fillRect(0, i, borderWidth, 4, borderBrown);
    canvas16.fillRect(0, i + 4, borderWidth, 4, borderGreen);
  }
  
  // Right border
  for(int i = 0; i < canvas16.height(); i += 8) {
    canvas16.fillRect(canvas16.width() - borderWidth, i, 4, borderWidth, borderBrown);
    canvas16.fillRect(canvas16.width() - borderWidth, i + 4, 4, borderWidth, borderGreen);
  }

  // Draw navigation arrows with high contrast
  int triangleHeight = 10;
  int offsetHeight = 10;
  int offsetWidth = 10;

  // Up arrow
  canvas16.fillTriangle(
    offsetWidth, triangleHeight + offsetHeight,
    triangleHeight + offsetWidth, triangleHeight + offsetHeight,
    triangleHeight/2 + offsetWidth, offsetHeight,
    ST77XX_WHITE
  );

  // Down arrow
  canvas16.fillTriangle(
    offsetWidth, tft.height()/2,
    offsetWidth + triangleHeight, tft.height()/2,
    offsetWidth + triangleHeight/2, tft.height()/2 + triangleHeight,
    ST77XX_WHITE
  );

  // Edit arrow
  canvas16.fillTriangle(
    offsetWidth, tft.height() - triangleHeight - offsetHeight,
    offsetWidth, tft.height() - offsetHeight,
    offsetWidth + triangleHeight, tft.height() - triangleHeight/2 - offsetHeight,
    editIndex == 0 ? ST77XX_WHITE : ST77XX_YELLOW
  );
}


unsigned long lastBatteryCheckTime = 0;
float lastBatteryVoltage = 0;
float lastBatteryPercentage = 0;
void updateDisplay(){
  updateInitialDisplay();

  unsigned long now = millis();


  //print the current time in the middle

  int timeOffset = editIndex == 0 ? 60 : 60;
  bool editingHours = editIndex == 1;
  bool editingMinutes = editIndex == 2;
  bool editingActive = editIndex == 3;

  canvas16.setCursor(timeOffset, tft.height()/2-20);
  canvas16.setTextColor(ST77XX_WHITE);
  canvas16.setTextSize(4);

  
  if(selectedIndex == 0) {
    // Display current time for clock
    if(editingHours){
      canvas16.setTextColor(ST77XX_YELLOW);
    }else{
      canvas16.setTextColor(ST77XX_WHITE);
    }

    // Calculate current time for display
    unsigned long now = millis();
    unsigned long elapsedMillis = now - startTime;
    unsigned long totalSeconds = elapsedMillis / 1000;
    unsigned long totalMinutes = totalSeconds / 60;
    unsigned long totalHours = totalMinutes / 60;
    
    currentSecond = totalSeconds % 60;
    currentMinute = (totalMinutes % 60 + minuteOffset) % 60;
    currentHour = (totalHours % 24 + hourOffset + (totalMinutes % 60 + minuteOffset) / 60) % 24;

    // Format hours with leading zero
    if (currentHour < 10) {
      canvas16.print("0");
    }
    canvas16.print(currentHour);

    canvas16.setTextColor(ST77XX_WHITE);
    canvas16.print(":");

    if(editingMinutes){
      canvas16.setTextColor(ST77XX_YELLOW);
    }else{
      canvas16.setTextColor(ST77XX_WHITE);
    }
    // Format minutes with leading zero
    if (currentMinute < 10) {
      canvas16.print("0");
    }
    canvas16.print(currentMinute);
    canvas16.print(" ");

    // Add seconds display
    canvas16.setTextSize(2);
    canvas16.setCursor(timeOffset, tft.height()/2+20);
    canvas16.setTextColor(ST77XX_WHITE);
    if (currentSecond < 10) {
      canvas16.print("0");
    }
    canvas16.print(currentSecond);
  } else {

    if(selectedIndex == menuSize-1){
      //display light debug mode: (you can turn on / off the light)
      canvas16.setTextSize(1);
      canvas16.setCursor(timeOffset, tft.height()/2+20);
      canvas16.setTextColor(ST77XX_WHITE);

      canvas16.setTextColor(ST77XX_GREEN);
      canvas16.print("Press up or down to trigger");

    }else{
      // Display alarm time
      if(editingHours){
        canvas16.setTextColor(ST77XX_YELLOW);
      }else{
        canvas16.setTextColor(ST77XX_WHITE);
      }

      if (hours[selectedIndex] < 10) {
        canvas16.print("0");
      }
      canvas16.print(hours[selectedIndex]);

      canvas16.setTextColor(ST77XX_WHITE);
      canvas16.print(":");

      if(editingMinutes){
        canvas16.setTextColor(ST77XX_YELLOW);
      }else{
        canvas16.setTextColor(ST77XX_WHITE);
      }
      if (minutes[selectedIndex] < 10) {
        canvas16.print("0");
      }
      canvas16.print(minutes[selectedIndex]);
      canvas16.print(" ");
    }
  }


  if(selectedIndex != 0 && selectedIndex != menuSize-1){
    canvas16.setTextSize(1);
    canvas16.setCursor(timeOffset, tft.height()-30);
    if(actives[selectedIndex]){
      if(editingActive){
        canvas16.setTextColor(ST77XX_YELLOW);
      }else{
        canvas16.setTextColor(ST77XX_GREEN);
      }
      canvas16.print("Active");
    }else{
      if(editingActive){
        canvas16.setTextColor(ST77XX_YELLOW);
      }else{
        canvas16.setTextColor(ST77XX_BLUE);
      }
      canvas16.print("Inactive");
    }
  }

  //print the alarm number on the bottom right (if 0, print "Clock")
  canvas16.setCursor(timeOffset, 30);
  canvas16.setTextColor(ST77XX_WHITE);
  canvas16.setTextSize(1);
  if(selectedIndex == 0){
    canvas16.print("Clock");
  }else{
    if(selectedIndex == menuSize-1){
      canvas16.print("Light Debug Mode");
    }else{
      canvas16.print("Alarm ");
      canvas16.print(selectedIndex);
    }
  }

  canvas16.drawRGBBitmap(0, 0, canvas16.getBuffer(), canvas16.width(), canvas16.height());



    



    if(now - lastBatteryCheckTime > 1000){


      lipo.wake();
      delay(100);
      lipo.quickStart();
      delay(100);

      lastBatteryVoltage = lipo.cellVoltage();
      lastBatteryPercentage = lipo.cellPercent();

      lastBatteryCheckTime = now;

      // Serial.println(lastBatteryVoltage);
      // Serial.println(lastBatteryPercentage);

      // Serial.print(F("(Dis)Charge rate : ")); Serial.print(lipo.chargeRate(), 1); Serial.println(" %/hr");


      // if (lipo.isHibernating()) {
      //   Serial.println(F("Hibernating!"));
      // }
    }

  drawBatteryIndicator(lastBatteryVoltage, lastBatteryPercentage);


  tft.drawRGBBitmap(0, 0, canvas16.getBuffer(), canvas16.width(), canvas16.height());

}


void testGraphics(){


  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

    delay(500);

     // large block of text
  tft.fillScreen(ST77XX_BLACK);
  testdrawtext(
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
      "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
      "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
      "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
      "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
      "tortor imperdiet posuere. ",
      ST77XX_WHITE);
  delay(1000);

    // tft print function!
  tftPrintTest();
  delay(4000);

  // a single pixel
  tft.drawPixel(tft.width() / 2, tft.height() / 2, ST77XX_GREEN);
  delay(500);

  // line draw test
  testlines(ST77XX_YELLOW);
  delay(500);

  // optimized lines
  testfastlines(ST77XX_RED, ST77XX_BLUE);
  delay(500);

  testdrawrects(ST77XX_GREEN);
  delay(500);

  testfillrects(ST77XX_YELLOW, ST77XX_MAGENTA);
  delay(500);

  tft.fillScreen(ST77XX_BLACK);
  testfillcircles(10, ST77XX_BLUE);
  testdrawcircles(10, ST77XX_WHITE);
  delay(500);

  testroundrects();
  delay(500);

  testtriangles();
  delay(500);

  mediabuttons();
  delay(500);

  Serial.println("done");
  delay(1000);

}






void testlines(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, 0, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, 0, 0, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, tft.height() - 1, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y = 0; y < tft.height(); y += 5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x = 0; x < tft.width(); x += 5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = tft.width() - 1; x > 6; x -= 6) {
    tft.fillRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color1);
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = radius; x < tft.width(); x += radius * 2) {
    for (int16_t y = radius; y < tft.height(); y += radius * 2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = 0; x < tft.width() + radius; x += radius * 2) {
    for (int16_t y = 0; y < tft.height() + radius; y += radius * 2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = tft.width() / 2;
  int x = tft.height() - 1;
  int y = 0;
  int z = tft.width();
  for (t = 0; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x -= 4;
    y += 4;
    z -= 4;
    color += 100;
  }
}

void testroundrects() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for (t = 0; t <= 4; t += 1) {
    int x = 0;
    int y = 0;
    int w = tft.width() - 2;
    int h = tft.height() - 2;
    for (i = 0; i <= 16; i += 1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x += 2;
      y += 3;
      w -= 4;
      h -= 6;
      color += 1100;
    }
    color += 100;
  }
}

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(4);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.print(p, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(" seconds.");
}

void mediabuttons() {
  // play
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(25, 5, 78, 60, 8, ST77XX_WHITE);
  tft.fillTriangle(42, 12, 42, 60, 90, 40, ST77XX_RED);
  delay(500);
  // pause
  tft.fillRoundRect(25, 70, 78, 60, 8, ST77XX_WHITE);
  tft.fillRoundRect(39, 78, 20, 45, 5, ST77XX_GREEN);
  tft.fillRoundRect(69, 78, 20, 45, 5, ST77XX_GREEN);
  delay(500);
  // play color
  tft.fillTriangle(42, 12, 42, 60, 90, 40, ST77XX_BLUE);
  delay(50);
  // pause color
  tft.fillRoundRect(39, 78, 20, 45, 5, ST77XX_RED);
  tft.fillRoundRect(69, 78, 20, 45, 5, ST77XX_RED);
  // play color
  tft.fillTriangle(42, 12, 42, 60, 90, 40, ST77XX_GREEN);
}

void drawBatteryIndicator(float voltage, float percentage) {
  // Battery icon dimensions
  const int batteryWidth = 30;
  const int batteryHeight = 15;
  const int batteryX = canvas16.width() - batteryWidth - 10; // 10px from right edge
  const int batteryY = 10; // 10px from top
  
  // Draw battery outline
  canvas16.drawRect(batteryX, batteryY, batteryWidth, batteryHeight, ST77XX_WHITE);
  canvas16.drawRect(batteryX + batteryWidth, batteryY + 3, 3, batteryHeight - 6, ST77XX_WHITE);
  
  // Calculate fill width based on percentage
  int fillWidth = (batteryWidth - 2) * (percentage / 100.0);
  
  // Choose color based on battery level
  uint16_t fillColor;
  if (percentage > 50) {
    fillColor = ST77XX_GREEN;
  } else if (percentage > 20) {
    fillColor = ST77XX_YELLOW;
  } else {
    fillColor = ST77XX_RED;
  }
  
  // Draw battery fill
  canvas16.fillRect(batteryX + 1, batteryY + 1, fillWidth, batteryHeight - 2, fillColor);
  
  // Draw percentage text
  canvas16.setTextSize(1);
  canvas16.setCursor(batteryX - 25, batteryY + 4);
  canvas16.setTextColor(ST77XX_WHITE);
  canvas16.print((int)percentage);
  canvas16.print("%");
}