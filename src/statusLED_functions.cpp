#include "statusLED_functions.h"

namespace {

StatusLedMode g_mode = StatusLedMode::NormalOff;
uint8_t g_monoPin = LED_BUILTIN;
bool g_ledLit = false;

#if defined(RGB_BUILTIN) && (!defined(STATUS_LED_FORCE_MONO) || (STATUS_LED_FORCE_MONO == 0))
const bool kHasRgbBuiltin = true;
#else
const bool kHasRgbBuiltin = false;
#endif

void writeMono(bool on) {
  g_ledLit = on;
  digitalWrite(g_monoPin, on ? HIGH : LOW);
}

void writeColor(uint8_t r, uint8_t g, uint8_t b) {
  g_ledLit = (r > 0 || g > 0 || b > 0);

#if defined(RGB_BUILTIN) && (!defined(STATUS_LED_FORCE_MONO) || (STATUS_LED_FORCE_MONO == 0))
  neopixelWrite(RGB_BUILTIN, r, g, b);
#else
  // Fallback for boards without RGB LED: any color maps to ON.
  writeMono(g_ledLit);
#endif
}

}  // namespace

void statusLedBegin(uint8_t fallbackPin) {
  g_monoPin = fallbackPin;
  pinMode(g_monoPin, OUTPUT);

#if defined(RGB_BUILTIN) && (!defined(STATUS_LED_FORCE_MONO) || (STATUS_LED_FORCE_MONO == 0))
  writeColor(0, 0, 0);
#else
  writeMono(false);
#endif
}

void statusLedSetMode(StatusLedMode mode) {
  g_mode = mode;
}

StatusLedMode statusLedGetMode() {
  return g_mode;
}

bool statusLedIsLit() {
  return g_ledLit;
}

void statusLedTick() {
  const uint32_t now = millis();

  switch (g_mode) {
    case StatusLedMode::NormalOff:
      writeColor(0, 0, 0);
      break;

    case StatusLedMode::WifiConnectedBreathGreen: {
      const uint32_t phase = now % 2000U;
      uint8_t level = 0;
      if (phase < 1000U) {
        level = static_cast<uint8_t>(phase / 16U);
      } else {
        level = static_cast<uint8_t>((2000U - phase) / 16U);
      }

      writeColor(0, level, 0);
      break;
    }

    case StatusLedMode::OtaUpdateAvailableBreathPurple: {
      const uint32_t phase = now % 3000U;
      uint8_t level = 0;
      if (phase < 1500U) {
        level = static_cast<uint8_t>(phase / 20U);
      } else {
        level = static_cast<uint8_t>((3000U - phase) / 20U);
      }

      // Purple = red + blue
      writeColor(level, 0, level);
      break;
    }

    case StatusLedMode::OtaInstallingSolidPurple:
      writeColor(64, 0, 64);
      break;

    case StatusLedMode::NoWifiBlinkBlueRed: {
      const bool phase = ((now / 300U) % 2U) == 0U;
      if (phase) {
        writeColor(0, 0, 48);  // blue
      } else {
        writeColor(48, 0, 0);  // red
      }
      break;
    }

    case StatusLedMode::NoInternetBlinkBlue: {
      const bool phase = ((now / 350U) % 2U) == 0U;
      if (phase) {
        writeColor(0, 0, 48);  // blue
      } else {
        writeColor(0, 0, 0);
      }
      break;
    }

    case StatusLedMode::ErrorBlinkRed: {
      const bool phase = ((now / 200U) % 2U) == 0U;
      if (phase) {
        writeColor(64, 0, 0);  // red
      } else {
        writeColor(0, 0, 0);
      }
      break;
    }
  }
}
