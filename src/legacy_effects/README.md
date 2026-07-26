# Legacy Effects Import

This folder contains imported source from the prior Arduino sketch (`LED_Controller_ESP-32_V2.2`) as a migration reference.

## Purpose
- Keep old effect implementations available while integrating into the new dual-core architecture.
- Port effect logic into `src/led/effects_engine.cpp` incrementally.
- Avoid running old direct `FastLED.show()` calls from web/network paths.

## Current integration model
- Web and network processing are on core 0.
- LED rendering runs in the LED task on core 1.
- Web API updates effect parameters; the LED task owns render/write timing.
