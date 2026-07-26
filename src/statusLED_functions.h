#pragma once

#include <Arduino.h>

enum class StatusLedMode : uint8_t {
  NormalOff = 0,
  WifiConnectedBreathGreen,
  OtaUpdateAvailableBreathPurple,
  NoWifiBlinkBlueRed,
  NoInternetBlinkBlue,
  ErrorBlinkRed,
};

void statusLedBegin(uint8_t fallbackPin);
void statusLedSetMode(StatusLedMode mode);
StatusLedMode statusLedGetMode();
void statusLedTick();
bool statusLedIsLit();
