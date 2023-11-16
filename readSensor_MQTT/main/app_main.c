#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

// CONNECT TO MQTT
static const char *TAG = "MQTT";

uint32_t MQTT_CONNEECTED = 0;
esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNEECTED = 1;

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNEECTED=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("From TOPIC = %.*s\r\n", event->topic_len, event->topic);
        printf("Receive DATA = %.*s\r\n", event->data_len, event->data);

        if(strncmp(event->topic, "/topic/test1", event->topic_len) == 0){
			if (strncmp(event->data, "1", event->data_len) == 0){
				gpio_set_level(2, 1); // Điều khiển LED bật
				printf("LED: On\n");
			} else if (strncmp(event->data, "0", event->data_len) == 0) {
				gpio_set_level(2, 0); // Điều khiển LED tắt
				printf("LED: Off\n");
			}
        }
        break;


    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}
static void mqtt_app_start(void)
{
	ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org:1883",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

//........................................................................
//........................................................................

// READ SENSOR
#define I2C_MASTER_SCL_IO 22 // SCL pin
#define I2C_MASTER_SDA_IO 21 // SDA pin
#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number
#define I2C_MASTER_FREQ_HZ 100000 // I2C frequency
#define I2C_MASTER_TX_BUF_DISABLE 0 // Disable TX buffer
#define I2C_MASTER_RX_BUF_DISABLE 0 // Disable RX buffer
#define WRITE_BIT I2C_MASTER_WRITE // I2C write bit
#define READ_BIT I2C_MASTER_READ // I2C read bit
#define ACK_CHECK_EN 0x1 // I2C ACK check enable
#define ACK_CHECK_DIS 0x0 // I2C ACK check disable
#define ACK_VAL 0x0 // I2C ACK value
#define NACK_VAL 0x1 // I2C NACK value

#define DHT12_ADDR 0x5C // DHT12 I2C address
#define DHT12_CMD 0x00 // DHT12 command register
#define DHT12_DATA_LEN 5 // DHT12 data length

#define BH1750_ADDR 0x23 // BH1750 I2C address
#define BH1750_CMD 0x10 // BH1750 command for continuous high resolution mode
#define BH1750_DATA_LEN 2 // BH1750 data length

#define FLAME_SENSOR_GPIO 39 // GPIO pin connected to the flame sensor

// Function to initialize I2C master
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Function to read DHT12 data
static esp_err_t i2c_master_read_dht12(i2c_port_t i2c_num, uint8_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DHT12_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, DHT12_CMD, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DHT12_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, DHT12_DATA_LEN, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Function to read BH1750 data
static esp_err_t i2c_master_read_bh1750(i2c_port_t i2c_num, uint8_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, BH1750_CMD, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS); // Wait for measurement to complete
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, BH1750_DATA_LEN, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
void Publisher_Task(void *params)
{
  while (true)
  {
    if(MQTT_CONNEECTED)
    {
    	int random_number = (esp_random() % 301) + 200;
    	float temperature = (float)random_number / 10;
    	char temperature_str[20];
    	snprintf(temperature_str, sizeof(temperature_str), "%.1f", temperature);
        esp_mqtt_client_publish(client, "/topic/test3", temperature_str, 0, 0, 0);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }
}
static void sensor_task(void *arg)
{
	while (true)
	{
		if(MQTT_CONNEECTED)
		{
			uint8_t dht12_data[DHT12_DATA_LEN];
			uint8_t bh1750_data[BH1750_DATA_LEN];
			int ret1, ret2, ret3;;
			float humidity, temperature, lux;

			ret1 = i2c_master_read_dht12(I2C_MASTER_NUM, dht12_data);
			ret2 = i2c_master_read_bh1750(I2C_MASTER_NUM, bh1750_data);

			// read flame sensor
		    gpio_config_t io_conf;
		    // Initialize the GPIO pin for the flame sensor
		    io_conf.intr_type = GPIO_INTR_DISABLE;
		    io_conf.mode = GPIO_MODE_INPUT;
		    io_conf.pin_bit_mask = (1ULL << FLAME_SENSOR_GPIO);
		    io_conf.pull_down_en = 0;
		    io_conf.pull_up_en = 1;
		    gpio_config(&io_conf);
		    if (gpio_get_level(FLAME_SENSOR_GPIO) == 1) {
				printf("No Flame Detected\n");
				ret3 = 0;
			} else {
				printf("Flame Detected!\n");
				ret3 = 1;
			}
		    ///////////////////
			if (ret1 != ESP_OK) {
				printf("Read DHT12 failed\n");
			}
			if(ret2 != ESP_OK)	{
				printf("Read BH1750 failed\n");
			}
			if (ret2 == ESP_OK && ret1 == ESP_OK) {
				humidity = dht12_data[0] + dht12_data[1] * 0.1;
				temperature = dht12_data[2] + (dht12_data[3] & 0x7F) * 0.1;
				if (dht12_data[3] & 0x80) {
					temperature = -temperature;
				}
				lux = ((bh1750_data[0] << 8) | bh1750_data[1]) / 1.2;
				char buffer[20];
				sprintf(buffer, "!%.1f:%.1f:%.2f:%d#\n", temperature, humidity, lux, ret3);
				esp_mqtt_client_publish(client, "/topic/test3", buffer, 0, 0, 0);
			}
			vTaskDelay(3000 / portTICK_PERIOD_MS);
		}
	}
}
//.............................................................
//.............................................................
void app_main(void)
{
	gpio_set_direction(2, GPIO_MODE_OUTPUT); // led for control in dashboard node-red
	gpio_set_level(2, 0);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());
    // Initialize I2C master
    i2c_master_init();
    // Create sensor task
    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 1024 * 2, NULL, 10, NULL, 1);
    mqtt_app_start();

}
