#pragma once
#include "EePub.h"
#include <iostream>

class EePubDesktop : public EePub {
public:
    EePubDesktop() {
        setDisplay(nullptr); // No actual display
    }
};
