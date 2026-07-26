#include "EffectRegistry.h"

namespace {

EffectDescriptor *g_head = nullptr;

}  // namespace

EffectRegistrar::EffectRegistrar(uint8_t id, const char *name, IEffect *effect) {
  static EffectDescriptor nodes[32];
  static uint8_t used = 0;

  if (used >= 32 || effect == nullptr || name == nullptr) {
    return;
  }

  EffectDescriptor *node = &nodes[used++];
  node->id = id;
  node->name = name;
  node->effect = effect;
  node->next = g_head;
  g_head = node;
}

EffectDescriptor *effectsRegistryHead() {
  return g_head;
}

EffectDescriptor *effectsFindById(uint8_t id) {
  EffectDescriptor *it = g_head;
  while (it != nullptr) {
    if (it->id == id) {
      return it;
    }
    it = it->next;
  }
  return nullptr;
}

uint8_t effectsCount() {
  uint8_t count = 0;
  EffectDescriptor *it = g_head;
  while (it != nullptr) {
    ++count;
    it = it->next;
  }
  return count;
}

String effectsCatalogJson() {
  EffectDescriptor *ordered[32] = {nullptr};
  uint8_t count = 0;

  EffectDescriptor *it = g_head;
  while (it != nullptr && count < 32) {
    ordered[count++] = it;
    it = it->next;
  }

  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = i + 1; j < count; ++j) {
      if (ordered[j]->id < ordered[i]->id) {
        EffectDescriptor *tmp = ordered[i];
        ordered[i] = ordered[j];
        ordered[j] = tmp;
      }
    }
  }

  String out = "[";
  for (uint8_t i = 0; i < count; ++i) {
    if (i > 0) {
      out += ",";
    }

    out += "{";
    out += "\"id\":" + String(ordered[i]->id);
    out += ",\"name\":\"" + String(ordered[i]->name) + "\"";
    out += "}";
  }

  out += "]";
  return out;
}
