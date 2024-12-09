# ESP32-Environment-Sensor
Environment sensor using a Bosch bme680 sensor and and ESP32 control board. Supports MQTT.


### MQTT Topics
  - **get_sensor_values**
    - Retrieve all sensors values
  - **get_iaq**
    - Retrive current IAQ reading
  - **get_gas**
    - Retrieve current VOC gas concentration
  - **get_temperature**
    - Retrive the current measured temperature 
  - **get_pressure**
    - Retrieve current pressure measurement 
  - **get_humidity**
   - Retrive current relative humidity
  - **update_hostname**
    - Update the network hostname of a specified device
  - **restart**
    - Restart a specified device
  - **reset**
    - Reset specified device to default parameters
  - **all/get_hostname**
    - Retrieve the hostname of all connected devices 
