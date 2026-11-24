#include <els/hardware_config.h>
#if ELS_SCREEN == ELS_SCREEN_ST7789_320X240_LVGL
#include "display.h"
#include "tft_transport.h"
#include <cstring>

bool Display::init() {
  initializeTft(tft, 240, 320);
  lv_init();
  m_screen = lv_display_create(320, 240);
  if (!m_screen) { m_failed = true; return false; }
  lv_display_set_color_format(m_screen, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(m_screen, m_pixels, nullptr, sizeof(m_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_user_data(m_screen, &tft);
  lv_display_set_flush_cb(m_screen, [](lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
    auto& driver = *static_cast<Adafruit_ST7789*>(lv_display_get_user_data(display));
    const auto w = lv_area_get_width(area), h = lv_area_get_height(area);
    driver.startWrite();
    driver.setAddrWindow(area->x1, area->y1, w, h);
    driver.writePixels(reinterpret_cast<uint16_t*>(pixels), w * h, true);
    driver.endWrite();
    lv_display_flush_ready(display);
  });

  auto* root = lv_display_get_screen_active(m_screen);
  lv_obj_set_style_bg_color(root, lv_color_hex(0x10191b), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  constexpr int y[] = {10, 43, 84, 111, 149, 175, 201};
  for (unsigned i = 0; i < 7; ++i) {
    auto* label = lv_label_create(root);
    if (!label) { m_failed = true; return false; }
    m_labels[i] = label;
    lv_obj_set_pos(label, 12, y[i]);
    lv_obj_set_style_text_font(label, i == 1 ? &lv_font_montserrat_24 : &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(i == 0 || i == 5 ? 0xb8dfd8 : 0xf6f3e8), 0);
    lv_label_set_text_static(label, m_lines[i]);
  }
  return true;
}

bool Display::service(uint32_t nowMs) {
  if (m_failed) return false;
  const auto elapsed = nowMs - m_lastServiceMs;
  if (elapsed >= 5) {
    m_lastServiceMs = nowMs;
    lv_tick_inc(elapsed);
    lv_timer_handler();
  }
  return true;
}

void Display::drawLines(const DisplayLines& lines) {
  for (unsigned i = 0; i < 7; ++i) {
    if (std::strcmp(lines[i], m_lines[i]) == 0) continue;
    std::strcpy(m_lines[i], lines[i]);
    lv_label_set_text_static(m_labels[i], m_lines[i]);
  }
}

void Display::showBootStatus(bool fault) {
  if (m_failed) return;
  DisplayLines lines{};
  std::strcpy(lines[0], fault ? "BOOT / FAULT" : "BOOT / LOCKED");
  drawLines(lines);
}

void Display::update() {
  if (m_failed) return;
  if (m_globalState.getDisplayReset()) {
    initializeTft(tft, 240, 320);
    lv_obj_invalidate(lv_display_get_screen_active(m_screen));
  }
  const auto status = readDisplayStatus(m_globalState, *m_spindle, *m_leadscrew);
  DisplayLines lines{};
  if (!composeText(lines, status)) formatDisplayLines(lines, status, nullptr);
  drawLines(lines);
}
#endif
