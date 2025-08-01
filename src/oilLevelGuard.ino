// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates Zigbee temperature and humidity sensor Sleepy device.
 *
 * The example demonstrates how to use Zigbee library to create an end device temperature and humidity sensor.
 * The sensor is a Zigbee end device, which is reporting data to the Zigbee network.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 *
 * Created by Jan Procházka (https://github.com/P-R-O-C-H-Y/)
 */


#ifndef ZIGBEE_MODE_ED
#
error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

/* Zigbee temperature + humidity sensor configuration */
#define TEMP_SENSOR_ENDPOINT_NUMBER 10

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  7190     // Sleep for 2 hours - 10 seconds     

uint8_t button = BOOT_PIN;

#define Liquid_Detection_Pin 0   //Input pin on sensor
#define RELAY_PIN 6              //Relay pin 

ZigbeeTempSensor zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);


/************************ Temp sensor *****************************/
void meausureAndSleep() {
  
  // Open the relay
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Relay opened");


  // wait for few seconds
  delay (5000);
  
  // Measure temperature sensor value
  float temperature = 0; //temperatureRead();
  if (digitalRead(Liquid_Detection_Pin)) {
    Serial.println("Liquid Detected!");
    temperature = 90.0;
  }
  else {
    Serial.println("No Liquid!");
    temperature = 0.0;
  }
  
  // Close the relay
  digitalWrite(RELAY_PIN, LOW);


  // add random to the temp in order to asure that the device will report
  srand(time(0));
  float tmp = 10.0*(float)rand()/(float)(RAND_MAX);
  temperature +=tmp;

  // Use temperature value as humidity value to demonstrate both temperature and humidity
  float humidity = temperature;

  // Update temperature and humidity values in Temperature sensor EP
  zbTempSensor.setTemperature(temperature);
  zbTempSensor.setHumidity(humidity);

  // Report temperature and humidity values
  zbTempSensor.report();
  Serial.printf("Reported temperature: %.2f°C, Humidity: %.2f%%\r\n", temperature, humidity);

  // Add small delay to allow the data to be sent before going to sleep
  delay(5000);

  // Put device to deep sleep
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

/********************* Arduino functions **************************/

void setup() {
  Serial.begin(115200);  
  Serial.println("Starting ...");
  
  //Zigbee.factoryReset();

  // Init button switch
  pinMode(button, INPUT_PULLUP);

  pinMode(Liquid_Detection_Pin, INPUT);

  // Set the relay pin as output
  pinMode(RELAY_PIN, OUTPUT);

  // Configure the wake up source and set to wake up every 5 seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Optional: set Zigbee device name and model
  zbTempSensor.setManufacturerAndModel("HomeMade", "LiquidDetector");

  // Set minimum and maximum temperature measurement value (10-50°C is default range for chip temperature measurement)
  zbTempSensor.setMinMaxValue(0, 100);

  // Set tolerance for temperature measurement in °C (lowest possible value is 0.01°C)
  zbTempSensor.setTolerance(1);

  // Set power source to battery and set battery percentage to measured value (now 100% for demonstration)
  // The value can be also updated by calling zbTempSensor.setBatteryPercentage(percentage) anytime
  zbTempSensor.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100);

  // Add humidity cluster to the temperature sensor device with min, max and tolerance values
  zbTempSensor.addHumiditySensor(0, 100, 1);

  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbTempSensor);

  // Create a custom Zigbee configuration for End Device with keep alive 10s to avoid interference with reporting data
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;

  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(&zigbeeConfig, false)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.println("Successfully connected to Zigbee network");

  // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
  delay(1000);
}


void loop() {
  // Checking button for factory reset
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
  }

  // Call the function to measure temperature and put the device to sleep
  meausureAndSleep();
}




