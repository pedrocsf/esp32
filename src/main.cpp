#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

static const char *TAG = "BLE_FAKE";

const char *device_names[60] = {
    "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903",
    // "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903", "HW706-0048903"
};

uint8_t device_macs[60][6] = {
    // {0xE7, 0x34, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x35, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x36, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x37, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x38, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x39, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3A, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3B, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3C, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3D, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3E, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x3F, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x40, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x41, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x42, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x43, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x44, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x45, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x46, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x47, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x48, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x49, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4A, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4B, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4C, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4D, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4E, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x4F, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x50, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x51, 0xF5, 0xC7, 0xD6, 0x44},
    {0xE7, 0x52, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x53, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x54, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x55, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x56, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x57, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x58, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x59, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5A, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5B, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5C, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5D, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5E, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x5F, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x60, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x61, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x62, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x63, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x64, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x65, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x66, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x67, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x68, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x69, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6A, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6B, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6C, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6D, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6E, 0xF5, 0xC7, 0xD6, 0x44},
    // {0xE7, 0x6F, 0xF5, 0xC7, 0xD6, 0x44}
};

int current_idx = 0;
bool address_set = false;
bool adv_data_set = false;
uint8_t current_bpm = 60;   // Valor do BPM atual (60-120)
bool bpm_increasing = true; // Flag para controlar direção do BPM

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

void start_advertising(const char *name)
{
  uint8_t adv_data[31];
  memset(adv_data, 0, sizeof(adv_data));
  int i = 0;

  // 1️⃣ FLAGS
  adv_data[i++] = 2;    // Length
  adv_data[i++] = 0x01; // Flags type
  adv_data[i++] = 0x06; // General Discoverable + BR/EDR Not Supported

  // 2️⃣ UUID do serviço 0x180D (Heart Rate)
  adv_data[i++] = 3;    // Length
  adv_data[i++] = 0x03; // Complete List of 16-bit Service UUIDs
  adv_data[i++] = 0x0D; // UUID LSB
  adv_data[i++] = 0x18; // UUID MSB

  // 3️⃣ Manufacturer Specific Data (com BPM real)
  adv_data[i++] = 8;    // Length (corrigido para 7 como na band real)
  adv_data[i++] = 0xFF; // Type = Manufacturer Specific
  adv_data[i++] = 0x05; // Company ID LSB
  adv_data[i++] = 0xFF; // Company ID MSB
  adv_data[i++] = 0x01; // Dados fixos
  adv_data[i++] = 0x4F;
  adv_data[i++] = 0x06;
  adv_data[i++] = current_bpm; // ⭐ BYTE DO BPM (era 0x00 na band real)
  adv_data[i++] = 0x0E;

  // 4️⃣ Nome do dispositivo (nome BLE)
  size_t name_len = strlen(name);
  if (name_len > 29 - i - 2) // -2 para type + length
    name_len = 29 - i - 2;
  adv_data[i++] = name_len + 1; // Length + type
  adv_data[i++] = 0x09;         // Complete Local Name type
  memcpy(&adv_data[i], name, name_len);
  i += name_len;

  // ⏺️ Enviar advertising data raw para o stack BLE
  esp_ble_gap_config_adv_data_raw(adv_data, i);
}

void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event)
  {
  case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT:
    ESP_LOGI(TAG, "Random address set");
    address_set = true;
    break;

  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    ESP_LOGI(TAG, "Advertising data set");
    adv_data_set = true;
    break;

  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
      ESP_LOGE(TAG, "Failed to start advertising");
    else
      ESP_LOGI(TAG, "Advertising started");
    break;

  default:
    break;
  }
}

void advertising_loop_task(void *param)
{
  while (1)
  {
    esp_ble_gap_stop_advertising();
    vTaskDelay(pdMS_TO_TICKS(1));

    address_set = false;
    adv_data_set = false;
    // Atualiza o BPM de forma oscilante entre 60 e 120
    if (bpm_increasing)
    {
      current_bpm++;
      if (current_bpm >= 120)
      {
        current_bpm = 120;
        bpm_increasing = false;
      }
    }
    else
    {
      current_bpm--;
      if (current_bpm <= 60)
      {
        current_bpm = 60;
        bpm_increasing = true;
      }
    }
    ESP_LOGI(TAG, ">> Device: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X, BPM: %d",
             device_names[current_idx],
             device_macs[current_idx][0], device_macs[current_idx][1],
             device_macs[current_idx][2], device_macs[current_idx][3],
             device_macs[current_idx][4], device_macs[current_idx][5],
             current_bpm);

    // ESP_LOGI(TAG, ">> Device: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
    //      device_names[current_idx],
    //      device_macs[current_idx][0], device_macs[current_idx][1],
    //      device_macs[current_idx][2], device_macs[current_idx][3],
    //      device_macs[current_idx][4], device_macs[current_idx][5]);

    esp_ble_gap_set_rand_addr(device_macs[current_idx]);

    while (!address_set)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    start_advertising(device_names[current_idx]);

    while (!adv_data_set)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_ble_gap_start_advertising(&adv_params);

    // Troca de dispositivo a cada 1 segundo, pode alterar aqui
    vTaskDelay(pdMS_TO_TICKS(1000));

    current_idx = (current_idx + 1) % 50;
  }
}

extern "C" void app_main(void)
{
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();

  esp_ble_gap_register_callback(gap_cb);

  xTaskCreate(advertising_loop_task, "ble_loop", 4096, NULL, 5, NULL);
}
