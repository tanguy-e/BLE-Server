#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_BMP280.h>

#define SERVICE_UUID "181A"
#define TEMPERATURE_CHAR_UUID "2A6E"
#define PRESSURE_CHAR_UUID "2A6D"
#define ALTITUDE_CHAR_UUID "ca73b3ba-39f6-4ab3-91ae-186dc9577d99"

Adafruit_BMP280 bmp;
BLEServer *pServer = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLECharacteristic *temperatureCharacteristic;
BLECharacteristic *pressureCharacteristic;
BLECharacteristic *altitudeCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void checkToReconnect()
{
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Reconnected");
    oldDeviceConnected = deviceConnected;
  }
}

void setup() {
  Serial.begin(115200);

  BLEDevice::init("TP_BLE_USERNAME");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("Démarrage de BLE fonctionne!");

  temperatureCharacteristic = pService->createCharacteristic(
    TEMPERATURE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  pressureCharacteristic = pService->createCharacteristic(
    PRESSURE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  altitudeCharacteristic = pService->createCharacteristic(
    ALTITUDE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
    Serial.println(F("Impossible de trouver un capteur BMP280, vérifier les connexions ou essayer une adresse différente!"));
    while (1) delay(10);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
}

void loop() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(1013.25);
  char dataStr[40];
  int value;

  checkToReconnect();

  value=(int)(temperature*100);
  temperatureCharacteristic->setValue(value);
  temperatureCharacteristic->notify();

  value=(int)(pressure*10);
  pressureCharacteristic->setValue(value);
  pressureCharacteristic->notify();

  sprintf(dataStr, "%.2f", altitude);
  altitudeCharacteristic->setValue(dataStr);
  altitudeCharacteristic->notify();

  Serial.print(F("Temperature = "));
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print(F("Pressure = "));
  Serial.print(pressure/100);
  Serial.println(" HPa");
  Serial.print(F("Altitude = "));
  Serial.print(altitude);
  Serial.println(" m");
  delay(10000);
}