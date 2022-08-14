/*
 * @Author: wing zlcnkkm@21cn.com
 * @Date: 2021-09-05 01:27:23
 * @LastEditors: wing zlcnkkm@21cn.com
 * @LastEditTime: 2022-08-13 21:22:20
 * @FilePath: /JoyShot/src/lamp.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 *  使用WS2812 LED 显示各种信息
 * 
 *  2021-09-05    wing    创建.
 */

#include <string.h>
#include "lamp.h"
#include "esp_log.h"

static const char *TAG = "Lamp";

static bool _is_on = false;

esp_err_t Lamp_Init(Lamp_Config_t *config) {
	ESP_LOGI(TAG, "Init");

	esp_err_t ret;
    // 使用LEDC 驱动CPC405x
    ledc_timer_config_t ledc_timer = {
		.speed_mode		 = CPC405X_PWM_SPEED_MODE,	// 速度模式
		.timer_num       = CPC405X_PWM_TIMER, 		// 定时器
		.duty_resolution = CPC405X_PWM_RESOLUTION, 	// 占空比分辨率
		.freq_hz 		 = CPC405X_PWM_FREQ,        // PWM 频率
		.clk_cfg 		 = LEDC_AUTO_CLK,//CPC405X_PWM_CLK_CFG,
	};
	ret = ledc_timer_config(&ledc_timer);
	if (ret != ESP_OK) return ret;

	ledc_channel_config_t ledc_channel = {
		.speed_mode = CPC405X_PWM_SPEED_MODE,
		.channel    = CPC405X_PWM_CHANNEL,
		.timer_sel  = CPC405X_PWM_TIMER,
        .intr_type	= LEDC_INTR_DISABLE,
		.gpio_num   = 25,
		.duty       = 0,
		.hpoint     = 0,
	};
    ret = ledc_channel_config(&ledc_channel);
	if (ret != ESP_OK) return ret;

	ret = ledc_set_duty(CPC405X_PWM_SPEED_MODE, CPC405X_PWM_CHANNEL, 4095);
	if (ret != ESP_OK) return ret;

	_is_on = false;
	

    // ESP_ERROR_CHECK(WS2812_Init(&config->ws2812));
    // vTaskDelay(config->ws2812.RES/ 1000 / portTICK_RATE_MS);
    // ESP_ERROR_CHECK(WS2812_Loop_Start(25));

	return WS2812_Init((WS2812_CONFIG_t *)WS2812_DEFAULT_CONFIG());
}

esp_err_t Lamp_On(uint32_t delay) {
	esp_err_t ret;

	ret = ledc_update_duty(CPC405X_PWM_SPEED_MODE, CPC405X_PWM_CHANNEL);
	if (ret != ESP_OK) return ret;

	_is_on = true;
	if (delay > 0) vTaskDelay(delay / portTICK_RATE_MS);
	return ESP_OK;
}

esp_err_t Lamp_Off(void) {
	_is_on = false;
	return ledc_stop(CPC405X_PWM_SPEED_MODE, CPC405X_PWM_CHANNEL, 0/*设置为低电平*/);
}

esp_err_t Lamp_Set_Color(WS2812_COLOR_t *color) {
	esp_err_t ret = ESP_OK;

	if (!_is_on) ret = Lamp_On(CPC405X_LDO_UP_DELAY);
	if (ret != ESP_OK) return ret;

	static WS2812_COLOR_t buf[WS2812_COUNT];
	for (size_t i = 0; i < WS2812_COUNT; i++)
	{
		memcpy(buf + i, color, sizeof(WS2812_COLOR_t));
	}
	
	return WS2812_Send_LEDs(buf, WS2812_COUNT);
}