#ifndef IR_REMOTE_HANDLER_H
#define IR_REMOTE_HANDLER_H

#include <IRremote.hpp>
#include <functional>

// IRRemoteHandler handles IR decoding and calls user-defined callbacks for press and hold
class IRRemoteHandler {
public:
    using ButtonCallback = std::function<void(uint32_t)>;

    // Constructor: takes the pin to which the IR receiver is connected
    IRRemoteHandler(uint16_t pin)
      : pin(pin), lastCode(0), lastPressTime(0),
        debounceDelay(50), holdDelay(500), 
        pressCallback(nullptr), holdCallback(nullptr) {}

    // Begin the receiver
    void begin() {
        IrReceiver.begin(pin, ENABLE_LED_FEEDBACK);  // Initialize the receiver
    }

    // Set the callback function for button press
    void setPressCallback(ButtonCallback cb) {
        pressCallback = cb;
    }

    // Set the callback function for button hold
    void setHoldCallback(ButtonCallback cb) {
        holdCallback = cb;
    }

    // Set the hold delay (time before a hold is considered)
    void setHoldDelay(unsigned long delayMs) {
        holdDelay = delayMs;
    }

    // Update method to be called in the loop
    void update() {
        unsigned long now = millis();

        if (IrReceiver.decode()) {
            uint32_t code = IrReceiver.decodedIRData.decodedRawData;

            // Handle a potential "hold" event by checking for the NEC repeat code
            if (code == 0xFFFFFFFF) {  // NEC repeat signal
                if ((now - lastPressTime) > holdDelay && holdCallback && lastCode != 0) {
                    holdCallback(lastCode);  // Call the hold callback
                    lastPressTime = now;     // Reset timer to allow repeatable hold event
                }
            } else {
                // Debounce: detect a new press
                if (code != lastCode || (now - lastPressTime) > debounceDelay) {
                    lastCode = code;
                    lastPressTime = now;

                    // Call the press callback
                    if (pressCallback) pressCallback(code);
                }
            }

            IrReceiver.resume();  // Prepare to receive the next code
        }
    }

private:
    uint16_t pin;                // The pin connected to the IR receiver
    uint32_t lastCode;           // Last decoded IR code to detect duplicates
    unsigned long lastPressTime; // Last time a button press was detected
    unsigned long holdDelay;     // Delay to recognize a hold action
    const unsigned long debounceDelay; // Delay to debounce multiple presses
    ButtonCallback pressCallback;  // Callback for button press
    ButtonCallback holdCallback;   // Callback for button hold
};

#endif
