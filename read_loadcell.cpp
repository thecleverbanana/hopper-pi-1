#include "loadcell.h"
#include <iostream>
#include <unistd.h>  // for usleep()

int main() {
    const char* CHIP_NAME = "gpiochip4";  // main Pi GPIO controller
    const int PIN_DT  = 5;  // GPIO5 (DT)
    const int PIN_SCK = 6;  // GPIO6 (SCK)

    LoadCell lc(CHIP_NAME, PIN_DT, PIN_SCK);
    if (!lc.initialize()) {
        std::cerr << "LoadCell init failed\n";
        return 1;
    }

    std::cout << "Reading load cell values...\n";

    while (true) {
        long raw = lc.read_raw();
        std::cout << "Raw value: " << raw << std::endl;
        usleep(200000);  // 200 ms delay between readings
    }

    return 0;
}
