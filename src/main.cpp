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
#define EEPROM_SIZE 64 // Aumentado para acomodar todas as configurações
#define EEPROM_ADDR 0
#define EEPROM_BPM_ADDR 1
#define EEPROM_DIR_ADDR 2
#define EEPROM_MODE_ADDR 3
#define EEPROM_SELECTED_MAC_ADDR 4
#define EEPROM_USE_CUSTOM_MAC_ADDR 5
#define EEPROM_CUSTOM_MAC_ADDR 6        // 6 bytes (6-11)
#define EEPROM_MAC_COUNT_ADDR 12        // Novo endereço para macCount
#define EEPROM_RESTART_INTERVAL_ADDR 16 // 4 bytes para unsigned long (16-19)

#define DEVICE_NAME "HW706-0047980"

// Variáveis de controle
bool autoRestart = true;
bool staticMode = false;
int selectedMacIndex = 0;
unsigned long restartInterval = 250; // Tempo em ms para reinicializações

unsigned long lastMenuCheck = 0;
const unsigned long MENU_CHECK_INTERVAL = 100;

// Variáveis para controle de tempo de boot
unsigned long bootStartTime = 0;
unsigned long bootCompleteTime = 0;
bool bootTimeRecorded = false;

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

// --- LISTA DE MACs UNICAST VÁLIDOS ---
int macCount = 99; // Mudança de const para variável modificável
uint8_t mac_list[99][6] = {
    {0xC2, 0x52, 0xF5, 0xC7, 0xD6, 0xFE},
    {0xD2, 0x4F, 0x3A, 0x77, 0x22, 0x10},
    {0xE2, 0x91, 0xAB, 0x12, 0x34, 0x54},
    {0xC6, 0x34, 0xA7, 0x89, 0x10, 0x20},
    {0xD6, 0x89, 0xB1, 0x43, 0x21, 0x8E},
    {0xE6, 0x11, 0x44, 0x66, 0x99, 0xA8},
    {0xC4, 0x12, 0x88, 0x55, 0x44, 0x64},
    {0xD4, 0xDE, 0xAD, 0xBE, 0xEF, 0xFE},
    {0xE4, 0x00, 0x11, 0x22, 0x33, 0x42},
    {0xC8, 0xA1, 0xB2, 0xC3, 0xD4, 0xE2},
    {0xD8, 0x55, 0x66, 0x77, 0x88, 0x96},
    {0xE8, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xCA, 0xBC, 0xDE, 0xF0, 0x12, 0x32},
    {0xDA, 0x45, 0x67, 0x89, 0xAB, 0xCA},
    {0xEA, 0x11, 0x22, 0x33, 0x44, 0x52},
    {0xCC, 0x9F, 0x8E, 0x7D, 0x6C, 0x58},
    {0xDC, 0x4A, 0x3B, 0x2C, 0x1D, 0x0C},
    {0xEC, 0xFE, 0xDC, 0xBA, 0x98, 0x74},
    {0xCE, 0x01, 0x23, 0x45, 0x67, 0x86},
    {0xF6, 0x5D, 0x5E, 0x5F, 0x60, 0x60},
    {0xCA, 0x61, 0x62, 0x63, 0x64, 0x64},
    {0xD2, 0x13, 0x24, 0x35, 0x46, 0x54},
    {0xE2, 0xF1, 0xE2, 0xD3, 0xC4, 0xB2},
    {0xC2, 0x11, 0x33, 0x55, 0x77, 0x96},
    {0xF2, 0x10, 0x20, 0x30, 0x40, 0x4E},
    {0xF6, 0x01, 0x02, 0x03, 0x04, 0x02},
    {0xF6, 0xAB, 0xCD, 0xEF, 0x12, 0x32},
    {0xF8, 0x55, 0xAA, 0x55, 0xAA, 0x52},
    {0xFA, 0x99, 0x00, 0x99, 0x00, 0x96},
    {0xFC, 0xDE, 0xAD, 0xFA, 0xCE, 0xFE},
    {0xFE, 0xCA, 0xFE, 0xBA, 0xBE, 0xFE},
    {0xC2, 0x11, 0x22, 0x33, 0x44, 0x52},
    {0xD2, 0x66, 0x77, 0x88, 0x99, 0xA8},
    {0xE2, 0x10, 0x32, 0x54, 0x76, 0x96},
    {0xC6, 0x89, 0x67, 0x45, 0x23, 0xFE},
    {0xD6, 0x0F, 0x1E, 0x2D, 0x3C, 0x48},
    {0xE6, 0x12, 0x34, 0x56, 0x78, 0x8E},
    {0xC4, 0x55, 0x44, 0x33, 0x22, 0x0E},
    {0xD4, 0xF0, 0xF1, 0xF2, 0xF3, 0xF2},
    {0xE4, 0x04, 0x03, 0x02, 0x01, 0xFE},
    {0xC8, 0x0A, 0x0B, 0x0C, 0x0D, 0x0C},
    {0xD8, 0xAA, 0xBB, 0xCC, 0xDD, 0xEC},
    {0xE8, 0x1A, 0x2B, 0x3C, 0x4D, 0x5C},
    {0xCA, 0xFE, 0xED, 0xBE, 0xEF, 0xFE},
    {0xDA, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xEA, 0x22, 0x44, 0x66, 0x88, 0xA8},
    {0xCC, 0x77, 0x66, 0x55, 0x44, 0x30},
    {0xDC, 0xAB, 0xCD, 0xEF, 0x01, 0x20},
    {0xEC, 0xBA, 0xDC, 0xFE, 0x10, 0x30},
    {0xCE, 0x01, 0x10, 0x01, 0x10, 0xFE},
    {0xDE, 0x42, 0x24, 0x42, 0x24, 0x40},
    {0xEE, 0x69, 0x96, 0x69, 0x96, 0x66},
    {0xC2, 0x0F, 0x0E, 0x0D, 0x0C, 0x08},
    {0xD2, 0xFF, 0xEE, 0xDD, 0xCC, 0xB8},
    {0xE2, 0x1B, 0x2C, 0x3D, 0x4E, 0x5C},
    {0xF2, 0xBE, 0xEF, 0xBE, 0xEF, 0xBC},
    {0xC2, 0x88, 0x99, 0xAA, 0xBB, 0xCA},
    {0xD2, 0x33, 0x44, 0x55, 0x66, 0x76},
    {0xE2, 0x77, 0x88, 0x99, 0xAA, 0xBA},
    {0xF2, 0x11, 0x22, 0x33, 0x44, 0x54},
    {0xC2, 0xAA, 0xBB, 0xCC, 0xDD, 0xEC},
    {0xD2, 0x12, 0x34, 0x56, 0x78, 0x98},
    {0xE2, 0xAB, 0xCD, 0xEF, 0x01, 0x20},
    {0xF2, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xC6, 0x11, 0x22, 0x33, 0x44, 0x54},
    {0xD6, 0xAA, 0xBB, 0xCC, 0xDD, 0xEC},
    {0xE6, 0x12, 0x34, 0x56, 0x78, 0x98},
    {0xF6, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xCA, 0x11, 0x22, 0x33, 0x44, 0x54},
    {0xDA, 0xAA, 0xBB, 0xCC, 0xDD, 0xEC},
    {0xEA, 0x12, 0x34, 0x56, 0x78, 0x98},
    {0xFA, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xCE, 0x11, 0x22, 0x33, 0x44, 0x54},
    {0xDE, 0xAA, 0xBB, 0xCC, 0xDD, 0xEC},
    {0xEE, 0x12, 0x34, 0x56, 0x78, 0x98},
    {0xFE, 0x99, 0x88, 0x77, 0x66, 0x52},
    {0xC2, 0x01, 0x02, 0x03, 0x04, 0x04},
    {0xD2, 0x05, 0x06, 0x07, 0x08, 0x08},
    {0xE2, 0x09, 0x0A, 0x0B, 0x0C, 0x0C},
    {0xF2, 0x0D, 0x0E, 0x0F, 0x10, 0x10},
    {0xC4, 0x11, 0x12, 0x13, 0x14, 0x14},
    {0xD4, 0x15, 0x16, 0x17, 0x18, 0x18},
    {0xE4, 0x19, 0x1A, 0x1B, 0x1C, 0x1C},
    {0xF4, 0x1D, 0x1E, 0x1F, 0x20, 0x20},
    {0xC8, 0x21, 0x22, 0x23, 0x24, 0x24},
    {0xD8, 0x25, 0x26, 0x27, 0x28, 0x28},
    {0xE8, 0x29, 0x2A, 0x2B, 0x2C, 0x2C},
    {0xF8, 0x2D, 0x2E, 0x2F, 0x30, 0x30},
    {0xCC, 0x31, 0x32, 0x33, 0x34, 0x34},
    {0xDC, 0x35, 0x36, 0x37, 0x38, 0x38},
    {0xEC, 0x39, 0x3A, 0x3B, 0x3C, 0x3C},
    {0xFC, 0x3D, 0x3E, 0x3F, 0x40, 0x40},
    {0xC2, 0x41, 0x42, 0x43, 0x44, 0x44},
    {0xD2, 0x45, 0x46, 0x47, 0x48, 0x48},
    {0xE2, 0x49, 0x4A, 0x4B, 0x4C, 0x4C},
    {0xF2, 0x4D, 0x4E, 0x4F, 0x50, 0x50},
    {0xC6, 0x51, 0x52, 0x53, 0x54, 0x54},
    {0xD6, 0x55, 0x56, 0x57, 0x58, 0x58},
    {0xE6, 0x59, 0x5A, 0x5B, 0x5C, 0x5C}};

// --- Função para pegar o próximo MAC index da EEPROM ---
int getNextMacIndex()
{
  int idx = EEPROM.read(EEPROM_ADDR);

  // Validação adicional
  if (idx >= macCount)
  {
    idx = 0;
  }

  idx = (idx + 1) % macCount;
  EEPROM.write(EEPROM_ADDR, idx);
  EEPROM.commit();

  Serial.printf("DEBUG: Próximo MAC index: %d (macCount: %d)\n", idx, macCount);
  return idx;
}

// Adicione estas variáveis após as outras variáveis de controle
uint8_t customMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool useCustomMac = false;
bool useRandomMac = false;

// --- Função para gerar MAC aleatório válido ---
void generateRandomMac(uint8_t *mac)
{
  // Primeiro byte: deve ser par para unicast e ter bit 1 setado para locally administered
  // Formato: xxxx xx10 (bits 0 e 1: 10 = locally administered unicast)
  mac[0] = (random(0, 64) << 2) | 0x02; // Garante que seja locally administered unicast

  // Evita alguns padrões problemáticos no primeiro byte
  while (mac[0] == 0x00 || mac[0] == 0xFF || (mac[0] & 0x01) == 1)
  {
    mac[0] = (random(0, 64) << 2) | 0x02;
  }

  // Outros 5 bytes podem ser qualquer valor válido
  for (int i = 1; i < 6; i++)
  {
    mac[i] = random(0, 256);
    // Evita valores problemáticos
    while (mac[i] == 0xFF && i < 5)
    { // Evita broadcast parcial
      mac[i] = random(0, 256);
    }
  }
}

// --- Função para exibir menu ---
void showMenu()
{
  Serial.println(u8R"rawliteral(
8888ba.88ba   .88888.   .88888.  dP     dP d8888888P 
88  `8b  `8b d8'   `8b d8'   `8b 88     88      .d8' 
88   88   88 88     88 88     88 88    .8P    .d8'   
88   88   88 88     88 88     88 88    d8'  .d8'     
88   88   88 Y8.   .8P Y8.   .8P 88  .d8P  d8'       
dP   dP   dP  `8888P'   `8888P'  888888'   Y8888888P 
                             )rawliteral");
  Serial.println("\n===========EMULADOR DE BANDS===========\n");
  Serial.println("'Criado para manter a sanidade dos devs'\n");
  Serial.println("\n--------------------MENU---------------------");
  Serial.println("1 - Modo Automático com lista pré-definida");
  Serial.println("2 - Modo Automático com MAC randômico");
  Serial.println("3 - Modo Estático");
  Serial.println("4 - Selecionar MAC específico da lista");
  Serial.println("5 - Digitar MAC customizado");
  Serial.println("6 - Listar MACs funcionais");
  Serial.println("7 - Mostrar status atual");
  Serial.println("8 - Definir quantidade de MACs da lista (1-99)");
  Serial.println("9 - Definir intervalo de restart (270-30000ms)");
  Serial.println("10 - Reiniciar dispositivo");
  Serial.println("----------------------------------------------");
  Serial.printf("MACs ativos: %d/99\n", macCount);
  Serial.printf("Intervalo restart: %lu ms\n", restartInterval);
  Serial.print("\nEscolha uma opção: ");
}

// --- Função para validar e converter MAC ---
bool parseCustomMac(String macStr, uint8_t *macArray)
{
  macStr.trim();
  macStr.toUpperCase();

  // Remove espaços
  macStr.replace(" ", "");

  // Verifica formato básico
  if (macStr.length() != 17)
  {
    return false;
  }

  // Verifica separadores
  if (macStr.charAt(2) != ':' || macStr.charAt(5) != ':' ||
      macStr.charAt(8) != ':' || macStr.charAt(11) != ':' ||
      macStr.charAt(14) != ':')
  {
    return false;
  }

  // Converte cada byte
  for (int i = 0; i < 6; i++)
  {
    String byteStr = macStr.substring(i * 3, i * 3 + 2);

    // Verifica se são caracteres hexadecimais válidos
    for (int j = 0; j < 2; j++)
    {
      char c = byteStr.charAt(j);
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
      {
        return false;
      }
    }

    // Converte string hexadecimal para byte
    macArray[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }

  return true;
}

// --- Função para listar MACs ---
void listMacs()
{
  Serial.println("\n=== LISTA DE MACs DISPONÍVEIS ===");
  for (int i = 0; i < macCount; i++)
  {
    Serial.printf("%02d: %02X:%02X:%02X:%02X:%02X:%02X\n", i,
                  mac_list[i][0], mac_list[i][1], mac_list[i][2],
                  mac_list[i][3], mac_list[i][4], mac_list[i][5]);
  }
  Serial.println("===============================");
}

// --- Função para mostrar status ---
void showStatus()
{
  Serial.println("\n=== STATUS ATUAL ===");

  // Mostra tempo de boot se disponível
  if (bootTimeRecorded)
  {
    unsigned long totalBootTime = bootCompleteTime - bootStartTime;
    Serial.printf("Último tempo de boot: %lu ms\n", totalBootTime);
  }

  if (autoRestart)
  {
    if (useRandomMac)
    {
      Serial.println("Modo: Automático com MAC Randômico");
    }
    else
    {
      Serial.println("Modo: Automático com Lista Pré-definida");
    }
    Serial.printf("Intervalo de restart: %lu ms\n", restartInterval);
  }
  else
  {
    Serial.println("Modo: Estático");
  }

  if (useCustomMac)
  {
    Serial.println("Tipo MAC: Customizado");
    Serial.printf("MAC Atual: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  customMac[0], customMac[1], customMac[2],
                  customMac[3], customMac[4], customMac[5]);
  }
  else if (useRandomMac && autoRestart)
  {
    Serial.println("Tipo MAC: Randômico");
    const uint8_t *currentMac = esp_bt_dev_get_address();
    Serial.printf("MAC Atual: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  currentMac[0], currentMac[1], currentMac[2],
                  currentMac[3], currentMac[4], currentMac[5]);
  }
  else
  {
    Serial.println("Tipo MAC: Da Lista");
    Serial.printf("MAC Index Atual: %d\n", selectedMacIndex);
    Serial.printf("MAC Atual: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac_list[selectedMacIndex][0], mac_list[selectedMacIndex][1], mac_list[selectedMacIndex][2],
                  mac_list[selectedMacIndex][3], mac_list[selectedMacIndex][4], mac_list[selectedMacIndex][5]);
  }

  // Mostra uptime atual
  unsigned long uptime = millis();
  Serial.printf("Tempo ativo: %lu ms (%.2f segundos)\n", uptime, uptime / 1000.0);

  Serial.println("==================");
}

// --- Função para processar comandos do menu ---
void processMenuCommand()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int choice = input.toInt();

    switch (choice)
    {
    case 1:
      autoRestart = true;
      staticMode = false;
      useCustomMac = false;
      useRandomMac = false;
      EEPROM.write(EEPROM_MODE_ADDR, 0);
      EEPROM.write(EEPROM_USE_CUSTOM_MAC_ADDR, 0);
      EEPROM.commit();
      Serial.println("Modo automático com lista pré-definida ativado! Reiniciando...");
      delay(restartInterval);
      esp_restart();
      break;

    case 2:
      autoRestart = true;
      staticMode = false;
      useCustomMac = false;
      useRandomMac = true;
      EEPROM.write(EEPROM_MODE_ADDR, 2); // Novo valor para modo randômico
      EEPROM.write(EEPROM_USE_CUSTOM_MAC_ADDR, 0);
      EEPROM.commit();
      Serial.println("Modo automático com MAC randômico ativado! Reiniciando...");
      delay(1000);
      esp_restart();
      break;

    case 3:
      autoRestart = false;
      staticMode = true;
      useRandomMac = false;
      EEPROM.write(EEPROM_MODE_ADDR, 1);
      EEPROM.commit();
      Serial.println("Modo estático ativado! Dispositivo não irá mais reiniciar automaticamente.");
      showMenu();
      break;

    case 4:
    {
      Serial.print("\n Digite o índice do MAC da lista (0-98): ");
      while (!Serial.available())
      {
        delay(10);
      }
      input = Serial.readStringUntil('\n');
      input.trim();
      int macIndex = input.toInt();
      if (macIndex >= 0 && macIndex < macCount)
      {
        selectedMacIndex = macIndex;
        useCustomMac = false;
        useRandomMac = false;

        // Salva configurações na EEPROM
        EEPROM.write(EEPROM_SELECTED_MAC_ADDR, selectedMacIndex);
        EEPROM.write(EEPROM_USE_CUSTOM_MAC_ADDR, 0);
        EEPROM.write(EEPROM_MODE_ADDR, 1); // Força modo estático
        EEPROM.commit();

        Serial.printf("MAC %d selecionado: %02X:%02X:%02X:%02X:%02X:%02X\n", macIndex,
                      mac_list[macIndex][0], mac_list[macIndex][1], mac_list[macIndex][2],
                      mac_list[macIndex][3], mac_list[macIndex][4], mac_list[macIndex][5]);
        Serial.println("Reiniciando para aplicar o novo MAC...");
        delay(1000);
        esp_restart();
      }
      else
      {
        Serial.println("Índice inválido! Use valores entre 0 e 98.");
      }
      showMenu();
      break;
    }

    case 5:
    {
      Serial.println("Digite o MAC no formato AA:BB:CC:DD:EE:FF");
      Serial.print("MAC customizado: ");
      while (!Serial.available())
      {
        delay(10);
      }
      input = Serial.readStringUntil('\n');

      if (parseCustomMac(input, customMac))
      {
        useCustomMac = true;
        staticMode = true;
        autoRestart = false;
        useRandomMac = false;

        // Salva configurações na EEPROM
        EEPROM.write(EEPROM_MODE_ADDR, 1);
        EEPROM.write(EEPROM_USE_CUSTOM_MAC_ADDR, 1);
        // Salva MAC customizado na EEPROM
        for (int i = 0; i < 6; i++)
        {
          EEPROM.write(EEPROM_CUSTOM_MAC_ADDR + i, customMac[i]);
        }
        EEPROM.commit();

        Serial.printf("MAC customizado válido: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      customMac[0], customMac[1], customMac[2],
                      customMac[3], customMac[4], customMac[5]);
        Serial.println("Reiniciando para aplicar o MAC customizado...");
        delay(1000);
        esp_restart();
      }
      else
      {
        Serial.println("Formato de MAC inválido! Use o formato AA:BB:CC:DD:EE:FF");
        Serial.println("Exemplo: C2:52:F5:C7:D6:00");
      }
      showMenu();
      break;
    }

    case 6:
      listMacs();
      showMenu();
      break;

    case 7:
      showStatus();
      showMenu();
      break;

    case 8:
    {
      Serial.printf("\nQuantidade atual de MACs: %d\n", macCount);
      Serial.print("Digite a nova quantidade (1-99): ");
      while (!Serial.available())
      {
        delay(10);
      }
      input = Serial.readStringUntil('\n');
      input.trim();
      int newCount = input.toInt();

      if (newCount >= 1 && newCount <= 99)
      {
        macCount = newCount;
        // Salva na EEPROM no endereço correto
        EEPROM.write(EEPROM_MAC_COUNT_ADDR, macCount);
        EEPROM.commit();

        Serial.printf("Quantidade de MACs definida para: %d\n", macCount);
        Serial.println("Agora o sistema usará apenas os primeiros " + String(macCount) + " MACs da lista.");

        // Se o MAC selecionado atual está fora do novo range, resetar para 0
        if (selectedMacIndex >= macCount)
        {
          selectedMacIndex = 0;
          EEPROM.write(EEPROM_SELECTED_MAC_ADDR, selectedMacIndex);
          EEPROM.commit();
          Serial.println("MAC selecionado resetado para índice 0 (fora do novo range).");
        }
      }
      else
      {
        Serial.println("Valor inválido! Use valores entre 1 e 99.");
      }
      showMenu();
      break;
    }

    case 9:
    {
      Serial.printf("\nIntervalo atual de restart: %lu ms\n", restartInterval);
      Serial.print("Digite o novo intervalo em ms: ");
      while (!Serial.available())
      {
        delay(10);
      }
      input = Serial.readStringUntil('\n');
      input.trim();
      unsigned long newInterval = input.toInt();

      if (newInterval >= 1 && newInterval <= 30000)
      {
        restartInterval = newInterval;
        // Salva na EEPROM usando put para unsigned long
        EEPROM.put(EEPROM_RESTART_INTERVAL_ADDR, restartInterval);
        EEPROM.commit();
        Serial.printf("Intervalo de restart definido para: %lu ms\n", restartInterval);
        Serial.println("Esta configuração foi salva e será aplicada no próximo modo automático.");
      }
      else
      {
        Serial.println("Valor inválido! Respeite o intervalo de tempo mínimo e máximo.");
      }
      showMenu();
      break;
    }

    case 10:
      Serial.println("Reiniciando dispositivo...");
      delay(1000);
      esp_restart();
      break;

    default:
      Serial.println("Opção inválida!");
      showMenu();
      break;
    }
  }
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
  // Marca o início do processo de boot
  bootStartTime = millis();

  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== INICIANDO BOOT ===");
  Serial.printf("Tempo de início: %lu ms\n", bootStartTime);

  EEPROM.begin(EEPROM_SIZE);

  // Carrega configurações da EEPROM
  uint8_t mode = EEPROM.read(EEPROM_MODE_ADDR);
  uint8_t storedMacIndex = EEPROM.read(EEPROM_SELECTED_MAC_ADDR);
  uint8_t storedUseCustom = EEPROM.read(EEPROM_USE_CUSTOM_MAC_ADDR);

  // Carrega quantidade de MACs da EEPROM
  uint8_t storedMacCount = EEPROM.read(EEPROM_MAC_COUNT_ADDR);
  if (storedMacCount >= 1 && storedMacCount <= 99)
  {
    macCount = storedMacCount;
    Serial.printf("Quantidade de MACs carregada: %d\n", macCount);
  }
  else
  {
    // Se não há valor válido, salva o padrão
    EEPROM.write(EEPROM_MAC_COUNT_ADDR, macCount);
    EEPROM.commit();
  }

  // Carrega intervalo de restart da EEPROM
  unsigned long storedInterval;
  EEPROM.get(EEPROM_RESTART_INTERVAL_ADDR, storedInterval);
  if (storedInterval >= 250 && storedInterval <= 30000)
  {
    restartInterval = storedInterval;
    Serial.printf("Intervalo de restart carregado: %lu ms\n", restartInterval);
  }
  else
  {
    // Se não há valor válido, salva o padrão
    EEPROM.put(EEPROM_RESTART_INTERVAL_ADDR, restartInterval);
    EEPROM.commit();
  }

  // Se não há valor válido na EEPROM, define modo padrão
  if (mode > 2)
  {
    mode = 1; // Modo estático por padrão para mostrar o menu
    EEPROM.write(EEPROM_MODE_ADDR, mode);
    EEPROM.commit();
  }

  // Carrega índice do MAC selecionado - CORRIGIDO
  if (storedMacIndex < macCount)
  {
    selectedMacIndex = storedMacIndex;
  }
  else
  {
    // Se índice inválido, resetar para 0
    selectedMacIndex = 0;
    EEPROM.write(EEPROM_SELECTED_MAC_ADDR, selectedMacIndex);
    EEPROM.commit();
    Serial.println("MAC index resetado para 0 (valor inválido na EEPROM)");
  }

  // Carrega configuração de MAC customizado
  if (storedUseCustom == 1)
  {
    useCustomMac = true;
    // Carrega MAC customizado da EEPROM
    for (int i = 0; i < 6; i++)
    {
      customMac[i] = EEPROM.read(EEPROM_CUSTOM_MAC_ADDR + i);
    }
  }

  Serial.println("--- Configurando modo de operação ---");
  if (mode == 1)
  {
    autoRestart = false;
    staticMode = true;
    useRandomMac = false;
    Serial.println("=== INICIANDO EM MODO ESTÁTICO ===");
  }
  else if (mode == 2)
  {
    autoRestart = true;
    staticMode = false;
    useCustomMac = false;
    useRandomMac = true;
    Serial.println("=== INICIANDO EM MODO AUTOMÁTICO RANDÔMICO ===");
    Serial.println(">>> Digite 'M' ou '2' a qualquer momento para voltar ao menu <<<");
  }
  else // mode == 0
  {
    autoRestart = true;
    staticMode = false;
    useCustomMac = false;
    useRandomMac = false;
    Serial.println("=== INICIANDO EM MODO AUTOMÁTICO COM LISTA ===");
    Serial.println(">>> Digite 'M' ou '2' a qualquer momento para voltar ao menu <<<");
  }

  Serial.println("--- Selecionando MAC ---");
  // --- Seleciona MAC ---
  uint8_t macToUse[6];
  if (staticMode && useCustomMac)
  {
    memcpy(macToUse, customMac, 6);
    Serial.println("Usando MAC customizado");
  }
  else if (staticMode)
  {
    memcpy(macToUse, mac_list[selectedMacIndex], 6);
    Serial.printf("Usando MAC da lista (índice %d)\n", selectedMacIndex);
  }
  else if (useRandomMac)
  {
    generateRandomMac(macToUse);
    Serial.println("Usando MAC randômico gerado");
  }
  else
  {
    int macIndex = getNextMacIndex();
    selectedMacIndex = macIndex;
    memcpy(macToUse, mac_list[macIndex], 6);
    Serial.printf("Usando MAC automático da lista (índice %d)\n", macIndex);
  }

  Serial.println("--- Configurando MAC base ---");
  esp_base_mac_addr_set(macToUse);

  Serial.println("--- Inicializando BLE ---");
  // --- Inicia BLE ---
  BLEDevice::init(DEVICE_NAME);

  const uint8_t *realMac = esp_bt_dev_get_address();
  Serial.printf("MAC BLE usado: %02X:%02X:%02X:%02X:%02X:%02X\n",
                realMac[0], realMac[1], realMac[2],
                realMac[3], realMac[4], realMac[5]);

  Serial.println("--- Criando servidor BLE ---");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  Serial.println("--- Criando serviços BLE ---");
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

  Serial.println("--- Configurando advertising ---");
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

  Serial.println("--- Iniciando advertising ---");
  // Inicia advertising
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();

  // Marca o fim do processo de boot e calcula o tempo total
  bootCompleteTime = millis();
  unsigned long totalBootTime = (bootCompleteTime - bootStartTime)/10;
  bootTimeRecorded = true;

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║           INFORMAÇÕES GERAIS           ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.printf("  Tempo de boot: %6lu ms               \n", totalBootTime);
  Serial.printf("  BPM atual: %8d                   \n", bpm);
  Serial.printf("  Bateria: %9d%%                   \n", battery);
  Serial.println("╚════════════════════════════════════════╝");

  if (staticMode)
  {
    Serial.println("\n=== MODO ESTÁTICO ATIVO ===");
    showMenu();
  }
  else
  {
    Serial.println("\nAdvertising iniciado - Modo Automático");
    Serial.printf("Próximo restart em %lu ms\n", restartInterval);
    Serial.println(">>> Digite 'M' ou '2' para acessar o menu <<<");
  }
}

bool checkForMenuRequest()
{
  if (Serial.available())
  {
    char input = Serial.read();
    // Limpa o buffer
    while (Serial.available())
    {
      Serial.read();
    }

    if (input == 'm' || input == 'M' || input == '2')
    {
      Serial.println("\n=== INTERROMPENDO MODO AUTOMÁTICO ===");
      autoRestart = false;
      staticMode = true;
      EEPROM.write(EEPROM_MODE_ADDR, 1);
      EEPROM.commit();
      Serial.println("Modo estático ativado!");
      showMenu();
      return true;
    }
  }
  return false;
}

void loop()
{
  if (staticMode)
  {
    // Modo estático - processa comandos do menu
    processMenuCommand();
    delay(100);
  }
  else
  {
    // Modo automático - mas verifica por comandos de menu periodicamente
    unsigned long currentTime = millis();

    if (currentTime - lastMenuCheck >= MENU_CHECK_INTERVAL)
    {
      if (checkForMenuRequest())
      {
        return;
      }
      lastMenuCheck = currentTime;
    }

    // Mostra countdown para o próximo restart
    unsigned long timeUntilRestart = restartInterval - (currentTime % restartInterval);
    if (timeUntilRestart <= 1000 && timeUntilRestart > 900) // Mostra apenas quando faltam ~1 segundo
    {
      Serial.printf("Reiniciando em %lu ms...\n", timeUntilRestart);
    }

    // Reinicia com o intervalo configurável
    vTaskDelay(pdMS_TO_TICKS(restartInterval));
    Serial.println("\n=== INICIANDO RESTART ===");
    esp_restart();
  }
}
