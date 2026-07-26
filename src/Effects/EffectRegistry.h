#pragma once

#include <Arduino.h>

#include "IEffect.h"

struct EffectDescriptor {
  uint8_t id;
  const char *name;
  IEffect *effect;
  EffectDescriptor *next;
};

class EffectRegistrar {
 public:
  EffectRegistrar(uint8_t id, const char *name, IEffect *effect);
};

EffectDescriptor *effectsRegistryHead();
EffectDescriptor *effectsFindById(uint8_t id);
uint8_t effectsCount();
String effectsCatalogJson();

#define REGISTER_EFFECT(CLASS_NAME, EFFECT_ID, EFFECT_LABEL) \
  namespace {                                                  \
  CLASS_NAME g_##CLASS_NAME##_instance;                        \
  EffectRegistrar g_##CLASS_NAME##_registrar(                 \
      static_cast<uint8_t>(EFFECT_ID), EFFECT_LABEL, &g_##CLASS_NAME##_instance); \
  }
