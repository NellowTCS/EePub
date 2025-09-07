#include <TFT_eSPI.h>
#include "EePubArduino.h"

TFT_eSPI tft = TFT_eSPI();
EePubArduino epub(tft);

void setup() {
    tft.init();
    tft.setRotation(1);
    epub.begin();
    epub.loadFile("/example.epub");
    epub.render();
}

void loop() {
    // Placeholder: navigation could go here
}
