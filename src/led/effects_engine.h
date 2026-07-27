#pragma once

#include <Arduino.h>

#include "../Effects/EffectTypes.h"

void effectsEngineBegin();
void effectsEngineTick();

LedEffectState effectsEngineGetState();
void effectsEngineSetPower(bool on);
void effectsEngineSetPattern(uint8_t pattern);
void effectsEngineSetBrightness(uint8_t brightness);
void effectsEngineSetSpeed(uint8_t speed);
void effectsEngineSetFps(uint16_t fps);
void effectsEngineSetDither(bool enabled);
void effectsEngineSetColor(uint8_t red, uint8_t green, uint8_t blue);
void effectsEngineSetActiveLedCount(uint16_t count);
uint16_t effectsEngineGetActiveLedCount();
uint16_t effectsEngineGetMaxLedCount();
uint16_t effectsEngineGetMaxFps();
uint16_t effectsEngineGetMaxFpsForLedCount(uint16_t ledCount);
String effectsEngineActiveSettingsSchemaJson();
String effectsEngineActiveSettingsStateJson();
bool effectsEngineSetActiveSetting(const String &key, const String &value);
void effectsEngineResetActiveSettings();
bool effectsEngineSaveActivePreset(uint8_t slot);
bool effectsEngineLoadActivePreset(uint8_t slot);
String effectsEngineActivePresetNamesJson();
bool effectsEngineSetActivePresetName(uint8_t slot, const String &name);

String effectsEngineStateJson();
String effectsEngineCatalogJson();
