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

String effectsEngineStateJson();
String effectsEngineCatalogJson();
