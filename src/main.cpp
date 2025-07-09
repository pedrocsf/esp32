#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <EEPROM.h>
#include "esp_bt_device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

// --- CONFIGS ---
#define EEPROM_SIZE 3
#define EEPROM_ADDR 0
#define EEPROM_BPM_ADDR 1
#define EEPROM_DIR_ADDR 2

#define DEVICE_NAME "HW706-0047980"
// --- LISTA DE MACs UNICAST VÁLIDOS ---
const int macCount = 8;
uint8_t getNextBPM()
{

  uint8_t bpm = EEPROM.read(EEPROM_BPM_ADDR);
  uint8_t dir = EEPROM.read(EEPROM_DIR_ADDR);

  // Inicialização de segurança
  if (bpm < 60 || bpm > 180)
    bpm = 60;
  if (dir > 1)
    dir = 0;

  // Atualiza BPM
  if (dir == 0) // Subindo
  {
    bpm++;
    if (bpm >= 180)
    {
      bpm = 180;
      dir = 1;
    }
  }
  else // Descendo
  {
    bpm--;
    if (bpm <= 60)
    {
      bpm = 60;
      dir = 0;
    }
  }

  // Grava valores atualizados
  EEPROM.write(EEPROM_BPM_ADDR, bpm);
  EEPROM.write(EEPROM_DIR_ADDR, dir);
  EEPROM.commit();

  return bpm;
}

uint8_t mac_list[8][6] = {

    {0xD2, 0x4F, 0x3A, 0x77, 0x22, 0x13}, //61
    {0xE2, 0x91, 0xAB, 0x12, 0x34, 0x56}, //62
    {0xCA, 0xBC, 0xDE, 0xF0, 0x12, 0x32}, //13
    {0xFA, 0x99, 0x88, 0x77, 0x66, 0x53}, //60
   // {0xF8, 0x0C, 0x0D, 0x0E, 0x0F, 0x0E}, //59
    {0xF6, 0xAA, 0xAA, 0xAA, 0xAA, 0xA8},  //58
    //{0xF6, 0xAA, 0xAA, 0xAA, 0xAA, 0xA8},
    //{0xF4, 0x12, 0x34, 0x56, 0x78, 0x98},

};

// --- Função para pegar o próximo MAC index da EEPROM ---
int getNextMacIndex()
{

  int idx = EEPROM.read(EEPROM_ADDR);
  idx = (idx + 1) % macCount;
  EEPROM.write(EEPROM_ADDR, idx);
  EEPROM.commit();
  return idx;
}

// --- Callbacks BLE (opcional) ---
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("Cliente conectado!");
  }
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Cliente desconectado!");
  }
};

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  // --- Seleciona MAC ---
  int macIndex = getNextMacIndex();
  uint8_t *customMac = mac_list[macIndex];

  // Força MAC válido (unicast, local)
  customMac[0] &= 0xFE; // bit 0 = 0
  customMac[0] |= 0x02; // bit 1 = 1

  esp_base_mac_addr_set(customMac);

  // --- Inicia BLE ---
  BLEDevice::init(DEVICE_NAME);

  const uint8_t *realMac = esp_bt_dev_get_address();
  Serial.printf("MAC usado: %02X:%02X:%02X:%02X:%02X:%02X\n",
                realMac[0], realMac[1], realMac[2],
                realMac[3], realMac[4], realMac[5]);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // --- Serviços (UUIDs) ---
  BLEService *heartRateService = pServer->createService(BLEUUID((uint16_t)0x180D));
  BLEService *userDataService = pServer->createService(BLEUUID((uint16_t)0x181C));
  BLEService *batteryService = pServer->createService(BLEUUID((uint16_t)0x180F));
  BLEService *deviceInfoService = pServer->createService(BLEUUID((uint16_t)0x180A));
  BLEService *customService = pServer->createService(BLEUUID((uint16_t)0xFD00));

  heartRateService->start();
  userDataService->start();
  batteryService->start();
  deviceInfoService->start();
  customService->start();

  // --- Advertising ---
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advData;

  advData.setFlags(0x06);

  // UUIDs no advertising
  uint8_t uuidData[] = {
      0x0B, 0x03,
      0x0D, 0x18,
      0x1C, 0x18,
      0x0F, 0x18,
      0x0A, 0x18,
      0x00, 0xFD};
  advData.addData(std::string((const char *)uuidData, sizeof(uuidData)));

  // Manufacturer data (opcional)
  uint8_t bpm = getNextBPM();
  uint8_t batteryLevels[] = {0, 25, 50, 75, 100};
  int index = random(0, 5);
  uint8_t battery = batteryLevels[index];
  uint8_t mfrData[] = {0x07, 0xFF, 0x05, 0xFF, 0x01, battery, 0x06, bpm};
  advData.addData(std::string((const char *)mfrData, sizeof(mfrData)));

  // Nome do dispositivo
  const char *nome = DEVICE_NAME;
  size_t nomeLength = strlen(nome);
  uint8_t nomeData[2 + nomeLength];
  nomeData[0] = 1 + nomeLength;
  nomeData[1] = 0x09; // Complete Local Name
  memcpy(&nomeData[2], nome, nomeLength);
  advData.addData(std::string((const char *)nomeData, sizeof(nomeData)));

  // Inicia advertising
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();
  Serial.print("BPM:");
  Serial.println(bpm);
  Serial.println("Advertising iniciado ###################################################################################################################");
}

void loop()
{
  // Nada a fazer aqui. Reinicie o ESP32 para trocar o MAC.

  while (true)
  {

    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
  }
}