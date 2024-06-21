#include "errLeds.h"

void errLeds(const int LED_BUILTIN, const int BLINK_TIME_MS)
{
    const int startTimeMS = millis();

    while((millis() - startTimeMS) < BLINK_TIME_MS){
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
}