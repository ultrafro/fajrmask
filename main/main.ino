//bismillah

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

// If you're using the full breakout...
//Adafruit_IS31FL3731 ledmatrix = Adafruit_IS31FL3731();
// If you're using the FeatherWing version
Adafruit_IS31FL3731_Wing ledmatrix = Adafruit_IS31FL3731_Wing();

// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);



// The lookup table to make the brightness changes be more visible
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

float p = 3.1415926;

const int menuSize = 11;
int selectedIndex = 0;
int editIndex = 0;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

String menuItems[menuSize] = {
  "Clock",
  "Alarm 1: 07:00 AM",
  "Alarm 2: 08:30 AM",
  "Alarm 3: 09:15 AM",
  "Alarm 4: 10:00 AM",
  "Alarm 5: 11:45 AM",
  "Alarm 6: 01:30 PM",
  "Alarm 7: 03:00 PM",
  "Alarm 8: 05:00 PM",
  "Alarm 9: 06:30 PM",
  "Alarm 10: 08:00 PM"
};

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

void setup() {
  Serial.begin(9600);
  Serial.println("ISSI swirl test");

  if (! ledmatrix.begin()) {
    Serial.println("IS31 not found");
    while (1);
  }
  Serial.println("IS31 found!");

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

  drawMenu();



  // uint16_t time = millis();
  // tft.fillScreen(ST77XX_BLACK);
  // time = millis() - time;

  //   delay(500);

  //    // large block of text
  // tft.fillScreen(ST77XX_BLACK);
  // testdrawtext(
  //     "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
  //     "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
  //     "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
  //     "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
  //     "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
  //     "tortor imperdiet posuere. ",
  //     ST77XX_WHITE);
  // delay(1000);

  //   // tft print function!
  // tftPrintTest();
  // delay(4000);

  // // a single pixel
  // tft.drawPixel(tft.width() / 2, tft.height() / 2, ST77XX_GREEN);
  // delay(500);

  // // line draw test
  // testlines(ST77XX_YELLOW);
  // delay(500);

  // // optimized lines
  // testfastlines(ST77XX_RED, ST77XX_BLUE);
  // delay(500);

  // testdrawrects(ST77XX_GREEN);
  // delay(500);

  // testfillrects(ST77XX_YELLOW, ST77XX_MAGENTA);
  // delay(500);

  // tft.fillScreen(ST77XX_BLACK);
  // testfillcircles(10, ST77XX_BLUE);
  // testdrawcircles(10, ST77XX_WHITE);
  // delay(500);

  // testroundrects();
  // delay(500);

  // testtriangles();
  // delay(500);

  // mediabuttons();
  // delay(500);

  // Serial.println("done");
  // delay(1000);


}

void loop() {

  bool shouldUpdateDisplay = false;

  bool redrawSelected = false;
  unsigned long now = millis();

  bool upPressed = digitalRead(0) == LOW && now - lastButtonPress>debounceDelay;
  bool downPressed = digitalRead(1) == HIGH && now - lastButtonPress>debounceDelay;
  bool editPressed = digitalRead(2) == HIGH && now - lastButtonPress>debounceDelay;

  if(upPressed){
    lastButtonPress = now;
  }

  if(downPressed){
    lastButtonPress = now;
  }

  if(editPressed){
    lastButtonPress = now;
    editIndex = (editIndex + 1)%5;
    shouldUpdateDisplay = true;
  }

  

  if(editIndex == 0){
    if(upPressed){
      selectedIndex = (selectedIndex - 1 + menuSize) % menuSize;
      redrawSelected = true;
      shouldUpdateDisplay = true;
    }

    if(downPressed){
      selectedIndex = (selectedIndex + 1) % menuSize;
      redrawSelected = true;
      shouldUpdateDisplay = true;
    }
  }

  if(editIndex == 1){
    if(upPressed){
      int newHour = hours[selectedIndex] +1;
      newHour = newHour % 24;
      hours[selectedIndex] = newHour;
      drawItem(selectedIndex);
    }

    if(downPressed){
      int newHour = hours[selectedIndex] -1;
      newHour = newHour % 24;
      hours[selectedIndex] = newHour;
      drawItem(selectedIndex);
    }
  }

  if(editIndex == 2){
    if(upPressed){
      int newMinutes = minutes[selectedIndex] +1;
      newMinutes = newMinutes % 60;
      minutes[selectedIndex] = newMinutes;
      drawItem(selectedIndex);
    }

    if(downPressed){
      int newMinutes = minutes[selectedIndex] -1;
      newMinutes = newMinutes % 60;
      minutes[selectedIndex] = newMinutes;
      drawItem(selectedIndex);
    }
  }




  // if (digitalRead(0) == LOW && now - lastButtonPress > debounceDelay) {
  //   selectedIndex = (selectedIndex - 1 + menuSize) % menuSize;
  //   lastButtonPress = now;
  //   redrawSelected = true;
  // }

  // if (digitalRead(1) == HIGH && now - lastButtonPress > debounceDelay) {
  //   selectedIndex = (selectedIndex + 1) % menuSize;
  //   lastButtonPress = now;
  //   redrawSelected = true;
  // }

  if(digitalRead(2) == HIGH){
    int i = 0;
    for (uint8_t x=0; x<16; x++) {
      for (uint8_t y=0; y<9; y++) {
        ledmatrix.drawPixel(x, y, i++);
     }
   }
  }else{
   int i = 0;
   for (uint8_t x=0; x<16; x++) {
     for (uint8_t y=0; y<9; y++) {
       ledmatrix.drawPixel(x, y, 0);
     }
   }    
  }

  if(redrawSelected){
    drawSelected();
  }






  // // animate over all the pixels, and set the brightness from the sweep table
  // for (uint8_t incr = 0; incr < 24; incr++)
  //   for (uint8_t x = 0; x < 16; x++)
  //     for (uint8_t y = 0; y < 9; y++)
  //       ledmatrix.drawPixel(x, y, sweep[(x+y+incr)%24]);
  delay(10);
}

void updateDisplay(){

}

void drawSelected(){
  tft.fillRect(0, 0, 20, tft.height(), ST77XX_BLACK);

  int triangleHeight = 16;
  int x0 = 0;
  int y0 = selectedIndex*12 -6;

  tft.fillTriangle(
     x0, y0,              // left point (base left)
     x0, y0 + triangleHeight, // bottom point (base right)
     x0 + 20, y0 + triangleHeight / 2, // right tip
     ST77XX_GREEN // or any color you want
  );

}

void drawItem(int idx){
  //fill in that section with black:
  tft.fillRect(30, 0, 100, 12, ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setCursor(30, idx * 12); // 12 pixels per line height
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  
  int hour = hours[idx];
  int minute = minutes[idx];
  char time_string[6];
  snprintf(time_string, sizeof(time_string), "%02d:%02d", hour, minute);

  tft.print(time_string);
}

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  for (int i = 0; i < menuSize; i++) {
    drawItem(i);
    // tft.setCursor(30, i * 12); // 12 pixels per line height
    // if (i == selectedIndex) {
    //   tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE); // Highlighted
    // } else {
    //   tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    // }
    // tft.print(menuItems[i]);
  }
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