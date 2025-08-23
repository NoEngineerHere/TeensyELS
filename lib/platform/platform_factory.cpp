#include "platform_abstraction.h"

#ifdef ESP32
#include "esp32_platform.h"
#else
#include "teensy_platform.h"
#endif

std::unique_ptr<IPlatformAbstraction> PlatformAbstractionFactory::create() {
#ifdef ESP32
    return std::unique_ptr<ESP32Platform>(new ESP32Platform());
#else
    return std::unique_ptr<TeensyPlatform>(new TeensyPlatform());
#endif
}