#include <TEWeatherShield.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

void TaskHTU21D(void *pvParameters);
void TaskMS5637(void *pvParameters);
void TaskMS8607(void *pvParameters);
void TaskTSD305(void *pvParameters);
void TaskTSY01(void *pvParameters);
void TaskPrint(void *pvParameters);

// Mutex lock for i2c bus
SemaphoreHandle_t gSensorMtx = NULL;

// weatherShield object
static TEWeatherShield gWeatherShield;

// Order: HTU21D, MS5637, MS8607, TSD305, TSY01
static float gTemperature[5];

// Order: HTU21D, MS5637, MS8607, TSD305, TSY01
static const int kTimeDelay[5] = { 20, 20, 20, 80, 15 };
static const char * kSensorString[5] = { "HTU21D", "MS5637", "MS8607", "TSD305", "TSY01" };

void setup() {
  // Initialize Serial bus
  Serial.begin(115200);

  // Initialize weather shield
  gWeatherShield.begin();

  while (!Serial) {
    // Going to wait here until we connect the serial terminal
  }

  xTaskCreate(
    TaskHTU21D,
    (const portCHAR *) "HTU21D",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskMS5637,
    (const portCHAR *) "MS5637",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskMS8607,
    (const portCHAR *) "MS8607",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskTSD305,
    (const portCHAR *) "TSD305",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskTSY01,
    (const portCHAR *) "TSY01",
    128,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TaskPrint,
    (const portCHAR *) "Printer",
    128,
    NULL,
    2,
    NULL
  );
}

void loop() {
  // Nothing happens here
}

void TaskHTU21D(void *pvParameters)
{
  float temperature, humidity;

  for(;;)
  {
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

      // Release mutex
      xSemaphoreGive(gSensorMtx);

      // Wait for ADC to settle before trying again
      vTaskDelay( kTimeDelay[0] / portTICK_PERIOD_MS );
    }
  }
}

void TaskMS5637(void *pvParameters)
{
  float temperature, pressure;

  for(;;)
  {
    // Check for mutex
    if(xSemaphoreTake(gSensorMtx, (TickType_t) 10) == pdTRUE) {
      // Select MS5637 on multiplexer
      gWeatherShield.selectMS5637();

      // Make sure this sensor is actually connected
      if (gWeatherShield.MS5637.is_connected()) {
        gWeatherShield.MS5637.read_temperature_and_pressure(&temperature, &pressure);

        gTemperature[1] = temperature;
      }

      // Release mutex
      xSemaphoreGive(gSensorMtx);

      // Wait for ADC to settle before trying again
      vTaskDelay( kTimeDelay[1] / portTICK_PERIOD_MS ); // wait for one second
    }
  }
}

void TaskMS8607(void *pvParameters)
{
  float temperature, pressure, humidity;

  for(;;)
  {
    if(xSemaphoreTake(gSensorMtx, (TickType_t) 10) == pdTRUE) {
      gWeatherShield.selectMS8607();

      // Make sure this sensor is actually connected
      if (gWeatherShield.MS8607.is_connected()) {
        gWeatherShield.MS8607.set_humidity_i2c_master_mode(ms8607_i2c_hold);
        gWeatherShield.MS8607.read_temperature_pressure_humidity(&temperature, &pressure, &humidity);

        gTemperature[2] = temperature;
      }

      // Release mutex
      xSemaphoreGive(gSensorMtx);

      // Wait for ADC to settle before trying again
      vTaskDelay( kTimeDelay[2] / portTICK_PERIOD_MS ); // wait for one second
    }
  }
}

void TaskTSD305(void *pvParameters)
{
  float temperature, object_temperature;

  for(;;)
  {
    if(xSemaphoreTake(gSensorMtx, (TickType_t) 10) == pdTRUE) {
      gWeatherShield.selectTSD305();

      // Make sure this sensor is actually connected
      if (gWeatherShield.TSD305.is_connected()) {
        gWeatherShield.TSD305.read_temperature_and_object_temperature(&temperature, &object_temperature);

        gTemperature[3] = temperature;
      }

      // Release mutex
      xSemaphoreGive(gSensorMtx);

      // Wait for ADC to settle before trying again
      vTaskDelay( kTimeDelay[3] / portTICK_PERIOD_MS ); // wait for one second
    }
  }
}

void TaskTSY01(void *pvParameters)
{
  float temperature;

  for(;;)
  {
    if(xSemaphoreTake(gSensorMtx, (TickType_t) 10) == pdTRUE) {
      gWeatherShield.selectTSYS01();

      // Make sure this sensor is actually connected
      if (gWeatherShield.TSYS01.is_connected()) {
        gWeatherShield.TSYS01.read_temperature(&temperature);

        gTemperature[4] = temperature;
      }

      // Release mutex
      xSemaphoreGive(gSensorMtx);

      // Wait for ADC to settle before trying again
      vTaskDelay( kTimeDelay[4] / portTICK_PERIOD_MS ); // wait for one second
    }
  }
}

void TaskPrint(void *pvParameters)
{
  for (;;)
  {
      for (int i = 0; i < 5; i++) {
        Serial.print("- ");
        Serial.print(kSensorString[i]);
        Serial.print(" : ");
        Serial.print(gTemperature[i]);
        Serial.print((char)176);
        Serial.println("C");
      }
      vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
  }
}
