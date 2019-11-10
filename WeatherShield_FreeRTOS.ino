#include <TEWeatherShield.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

void TaskSensorRead(void *pvParameters);
void TaskEstimator(void *pvParameters);

// Mutex lock for i2c bus
SemaphoreHandle_t gSensorMtx = NULL;
SemaphoreHandle_t gSensorSemaphore = NULL;

// weatherShield object
static TEWeatherShield gWeatherShield;

// Order: HTU21D, MS5637, MS8607, TSD305, TSY01
static float gTemperature[5] { 0, 0, 0, 0 , 0 };
static float gEstimate = 0.0;
static const char * kSensorString[5] = { "HTU21D", "MS5637", "MS8607", "TSD305", "TSY01" };
static const float kVarianceInverse[5] = { 1 / 0.01, 1 / 0.01, 1 / 0.01, 1 / 0.01, 1 / 0.01 };
static float kVarianceInverseSum = 0.0;

void setup() {
  // Initialize Serial bus
  Serial.begin(115200);

  // Initialize weather shield
  gWeatherShield.begin();

  // Sum variance kVarianceInverse
  for (int i=0; i<5; i++) {
    kVarianceInverseSum += kVarianceInverse[i];
  }

  while (!Serial) {
    // Going to wait here until we connect the serial terminal
  }

  gSensorMtx = xSemaphoreCreateMutex();
  gSensorSemaphore = xSemaphoreCreateBinary();

  xTaskCreate(
    TaskSensorRead,
    (const portCHAR *) "HTU21D",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskEstimator,
    (const portCHAR *) "Estimator",
    128,
    NULL,
    2,
    NULL
  );
}

void loop() {
  // Nothing happens here
}

void TaskSensorRead(void *pvParameters)
{
  float temperature, humidity, pressure, object_temperature;

  for(;;)
  {
    if ( gSensorMtx != NULL) {
      // Check for mutex
      if(xSemaphoreTake(gSensorMtx, (TickType_t) 10) == pdTRUE) {
        // Select HTU21D on multiplexer
        gWeatherShield.selectHTU21D();
  
        // Make sure this sensor is actually connected
        if (gWeatherShield.HTU21D.is_connected()) {
          gWeatherShield.HTU21D.set_i2c_master_mode(htu21_i2c_no_hold);
          gWeatherShield.HTU21D.read_temperature_and_relative_humidity(&temperature, &humidity);
  
          gTemperature[0] = temperature;
        }

        // Select MS5637 on multiplexer
        gWeatherShield.selectMS5637();

        // Make sure this sensor is actually connected
        if (gWeatherShield.MS5637.is_connected()) {
          gWeatherShield.MS5637.read_temperature_and_pressure(&temperature, &pressure);

          gTemperature[1] = temperature;
        }

        gWeatherShield.selectMS8607();

        // Make sure this sensor is actually connected
        if (gWeatherShield.MS8607.is_connected()) {
          gWeatherShield.MS8607.set_humidity_i2c_master_mode(ms8607_i2c_hold);
          gWeatherShield.MS8607.read_temperature_pressure_humidity(&temperature, &pressure, &humidity);

          gTemperature[2] = temperature;
        }

        gWeatherShield.selectTSD305();

        // Make sure this sensor is actually connected
        if (gWeatherShield.TSD305.is_connected()) {
          gWeatherShield.TSD305.read_temperature_and_object_temperature(&temperature, &object_temperature);

          gTemperature[3] = temperature;
        }

        gWeatherShield.selectTSYS01();

        // Make sure this sensor is actually connected
        if (gWeatherShield.TSYS01.is_connected()) {
          gWeatherShield.TSYS01.read_temperature(&temperature);

          gTemperature[4] = temperature;
        }

        // Release mutex
        xSemaphoreGive(gSensorMtx);
        // Release Semaphore
        xSemaphoreGive(gSensorSemaphore);
  
        // half second delay
        vTaskDelay( 100 / portTICK_PERIOD_MS );
      }
    }
  }
}

void TaskEstimator(void *pvParameters)
{
  for(;;)
  {
    if (gSensorSemaphore != NULL) {
      if (xSemaphoreTake(gSensorSemaphore, (TickType_t) 10) == pdTRUE) {
        for (int i=0; i<5; i++) {
          gEstimate += kVarianceInverse[i] * gTemperature[i];
        }
        gEstimate /= kVarianceInverseSum;

        for (int i=0; i<5; i++)
        {
          Serial.print(gTemperature[i]);
          Serial.print(", ");
        }
        Serial.print(gEstimate);
        Serial.println();
      }
    }
  }
}
