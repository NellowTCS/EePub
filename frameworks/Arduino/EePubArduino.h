#pragma once
#include "EePub.h"
#include <TFT_eSPI.h>

class EePubArduino : public EePub {
public:
    EePubArduino(TFT_eSPI& display) : display(display) {
        EePub::pImpl->renderer.setDisplay(&display);
    }

    void render() {
        display.fillScreen(TFT_BLACK);
        EePub::render(); // Calls core renderer
    }

private:
    TFT_eSPI& display;
};
