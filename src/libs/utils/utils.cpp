#include "utils.h"

float celsiusToFahrenheit(float celsius)
{
    return ((celsius * 1.8) + 32);
}

float calcAltitude(float pressure)
{
    const float sea_level_pressure = 1010;                             // hPa (Auburn - 500m)
    pressure = pressure * 0.01;                                        // Convert Pa to hPa
    return 44330 * (1.0 - pow(pressure / sea_level_pressure, 0.1903)); // Altitude in meters (https://github.com/adafruit/Adafruit_CircuitPython_BME680/)
}