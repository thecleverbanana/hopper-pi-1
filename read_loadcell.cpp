#include "loadcell.h"
#include <iostream>

int main() {
    const int PIN_DT  = 5;  // GPIO5
    const int PIN_SCK = 6;  // GPIO6

    LoadCell lc(PIN_DT, PIN_SCK);
    if (!lc.initialize()) {
        std::cerr << "LoadCell init failed\n";
        return 1;
    }

    long raw = lc.read_raw();
    std::cout << "LoadCell raw value: " << raw << std::endl;
    return 0;
}
