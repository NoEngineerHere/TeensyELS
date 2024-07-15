#include "display.h"

// Images
#include "feedSymbol.h"
#include "lockedSymbol.h"
#include "pauseSymbol.h"
#include "runSymbol.h"
#include "threadSymbol.h"
#include "unlockedSymbol.h"

void Display::init() {
#ifdef ELS_DISPLAY == SSD1306_128_64
  if (!this->m_ssd1306.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  m_ssd1306.clearDisplay();
#endif
}

void Display::update(boolean driveMode, float pitch, boolean lock,
                     boolean enabled, boolean jogging, boolean syncState, boolean startPointSet) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.clearDisplay();
#endif

  this->drawMode(driveMode);
  this->drawPitch(pitch);
  this->drawLocked(lock);
  this->drawEnabled(enabled); 
  this->drawSyncState(syncState);
  this->drawThreadStartPointSet(startPointSet);
  this->drawJogging(jogging);
  

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.display();
#endif
}

void Display::drawMode(boolean driveMode) {
#if ELS_DISPLAY == SSD1306_128_64
  if (driveMode == true) {
    
    m_ssd1306.drawBitmap(57, 32, threadSymbol, 64, 32, WHITE);
  } else {
    m_ssd1306.drawBitmap(57, 32, feedSymbol, 64, 32, WHITE);
  }
#endif
}

void Display::drawPitch(float pitch) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.setCursor(55, 6);
  m_ssd1306.setTextSize(3);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(pitch);
#endif
}

void Display::drawEnabled(boolean enabled) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(26, 40, 20, 20, 2, WHITE);

  if (enabled == true) {
    m_ssd1306.drawBitmap(28, 42, runSymbol, 16, 16, BLACK);
  } else {
    m_ssd1306.drawBitmap(28, 42, pauseSymbol, 16, 16, BLACK);
  }
#endif
}

void Display::drawLocked(boolean locked) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(2, 40, 20, 20, 2, WHITE);

  if (locked == true) {
    m_ssd1306.drawBitmap(4, 42, lockedSymbol, 16, 16, BLACK);
  } else {
    m_ssd1306.drawBitmap(4, 42, unlockedSymbol, 16, 16, BLACK);
  }
#endif
}

void Display::drawJogging(boolean jogging) {
#if ELS_DISPLAY == SSD1306_128_64
 

  if (jogging == true) {
      m_ssd1306.fillRoundRect(30, 14, 68, 36, 12, WHITE);
      m_ssd1306.fillRoundRect(32, 16, 64, 32, 10, BLACK);
    
      m_ssd1306.setCursor(37, 22);
      m_ssd1306.setTextSize(3);
      m_ssd1306.setTextColor(WHITE);
      m_ssd1306.print("JOG");
    }
  
  
#endif
}

void Display::drawSyncState(boolean syncState) {
#if ELS_DISPLAY == SSD1306_128_64
  

  if (syncState == true) {
      m_ssd1306.setCursor(2, 2);
      m_ssd1306.setTextSize(2);
      m_ssd1306.setTextColor(WHITE);
      m_ssd1306.print("SYNC");
  }

  else
  {
    m_ssd1306.setCursor(2, 2);
      m_ssd1306.setTextSize(2);
      m_ssd1306.setTextColor(WHITE);
      m_ssd1306.print("SYNC");
      m_ssd1306.fillRoundRect(0, 8, 50, 2, 1, WHITE);
  }
  
#endif
}

void Display::drawThreadStartPointSet(boolean startPointSet) {
#if ELS_DISPLAY == SSD1306_128_64


if (startPointSet == true) {
    m_ssd1306.setCursor(2, 21);
    m_ssd1306.setTextSize(2);
    m_ssd1306.setTextColor(WHITE);
    m_ssd1306.print("STRT");
}

else
{
    m_ssd1306.setCursor(2, 21);
    m_ssd1306.setTextSize(2);
    m_ssd1306.setTextColor(WHITE);
    m_ssd1306.print("STRT");
    m_ssd1306.fillRoundRect(0, 27, 50, 2, 1, WHITE);
    
}
#endif

}






