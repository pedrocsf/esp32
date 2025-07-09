// #include <Arduino.h>
// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLEAdvertising.h>

// class MyServerCallbacks : public BLEServerCallbacks
// {
//     void onConnect(BLEServer *pServer)
//     {
//         Serial.println("Cliente conectado!");
//     }
//     void onDisconnect(BLEServer *pServer)
//     {
//         Serial.println("Cliente desconectado!");
//     }
// };

// void setup()
// {
//     Serial.begin(115200);
//     BLEDevice::init("HW706-0047980");

//     BLEServer *pServer = BLEDevice::createServer();
//     pServer->setCallbacks(new MyServerCallbacks());

//     // CriaÃ§Ã£o dos serviÃ§os como antes (180D, 181C, 180F, 180A, FD00)
//     BLEService *heartRateService = pServer->createService(BLEUUID((uint16_t)0x180D));
//     BLEService *userDataService = pServer->createService(BLEUUID((uint16_t)0x181C));
//     BLEService *batteryService = pServer->createService(BLEUUID((uint16_t)0x180F));
//     BLEService *deviceInfoService = pServer->createService(BLEUUID((uint16_t)0x180A));
//     BLEService *customService = pServer->createService(BLEUUID((uint16_t)0xFD00));

//     heartRateService->start();
//     userDataService->start();
//     batteryService->start();
//     deviceInfoService->start();
//     customService->start();

//     BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
//     BLEAdvertisementData advData;

//     advData.setFlags(0x06);

//     uint8_t uuidData[] = {
//         0x0B,
//         0x03,
//         0x0D, 0x18,
//         0x1C, 0x18,
//         0x0F, 0x18,
//         0x0A, 0x18,
//         0x00, 0xFD};
//     advData.addData(std::string((const char *)uuidData, sizeof(uuidData)));

//     uint8_t mfrData[] = {0x07, 0xFF, 0x05, 0xFF, 0x01, 0x4F, 0x06, 0x67};
//     advData.addData(std::string((const char *)mfrData, sizeof(mfrData)));

//     const char *nome = "HW706-0047980";
//     size_t nomeLength = strlen(nome);
//     uint8_t nomeData[2 + nomeLength];
//     nomeData[0] = 1 + nomeLength; // Length of this field
//     nomeData[1] = 0x09;           // Complete Local Name AD type
//     memcpy(&nomeData[2], nome, nomeLength);
//     advData.addData(std::string((const char *)nomeData, sizeof(nomeData)));

//     pAdvertising->setAdvertisementData(advData);
//     pAdvertising->start();

//     Serial.println("Advertising iniciado. Aguardando conexÃ£o...");
// }

// void loop()
// {
//     // nada aqui
// }
