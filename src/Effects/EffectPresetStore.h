#pragma once

#include <Preferences.h>

#include <stdint.h>
#include <stdio.h>

inline bool saveEffectPresetBytes(uint8_t effectId, uint8_t slot, const uint8_t *data, size_t length) {
  if (slot < 1 || slot > 5 || data == nullptr || length == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin("fxpreset", false)) {
    return false;
  }

  char key[16] = {0};
  snprintf(key, sizeof(key), "e%us%u", effectId, slot);
  const size_t written = prefs.putBytes(key, data, length);
  prefs.end();

  return written == length;
}

inline bool loadEffectPresetBytes(uint8_t effectId, uint8_t slot, uint8_t *data, size_t length) {
  if (slot < 1 || slot > 5 || data == nullptr || length == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin("fxpreset", true)) {
    return false;
  }

  char key[16] = {0};
  snprintf(key, sizeof(key), "e%us%u", effectId, slot);
  const size_t read = prefs.getBytes(key, data, length);
  prefs.end();

  return read == length;
}

inline bool saveEffectPresetName(uint8_t effectId, uint8_t slot, const String &name) {
  if (slot < 1 || slot > 5) {
    return false;
  }

  String trimmed = name;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return false;
  }
  if (trimmed.length() > 24) {
    trimmed = trimmed.substring(0, 24);
  }

  Preferences prefs;
  if (!prefs.begin("fxpreset", false)) {
    return false;
  }

  char key[16] = {0};
  snprintf(key, sizeof(key), "n%us%u", effectId, slot);
  const size_t written = prefs.putString(key, trimmed);
  prefs.end();

  return written > 0;
}

inline String loadEffectPresetName(uint8_t effectId, uint8_t slot) {
  if (slot < 1 || slot > 5) {
    return String();
  }

  Preferences prefs;
  if (!prefs.begin("fxpreset", true)) {
    return String();
  }

  char key[16] = {0};
  snprintf(key, sizeof(key), "n%us%u", effectId, slot);
  const String value = prefs.getString(key, "");
  prefs.end();
  return value;
}