/**
 *  CPC405x 电源管理驱动
 *  
 *  2022-08-18    wing    创建.
 */

#include <string.h>
#include "cpc405x.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CPC405X";

typedef struct {
	struct  {
		bool				initialized : 1;	// 是否初始化过
		bool 				is_on 		: 1;	// 开关状态
	};
	
	CPC405x_LDO_Config_t 	config;
} _LDO_t;


// CPC405X 系列目前最多有两路可调LDO
static _LDO_t _ldo[2] = {
	{.initialized = false, .is_on = false},
	{.initialized = false, .is_on = false},
};


esp_err_t CPC405x_LDO_Init(CPC405x_LDO_Config_t *config) {
	ESP_LOGI(TAG, "LDO Init");

	ESP_ERROR_CHECK(config == NULL);
    ESP_ERROR_CHECK(config->ldo_num == CPC405x_LDO_None || config->ldo_num >= CPC405x_LDO_Max);

	memcpy(&(_ldo[config->ldo_num - 1].config), config, sizeof(CPC405x_LDO_Config_t));

	esp_err_t ret;
    // 使用LEDC 驱动CPC405x
    ledc_timer_config_t ledc_timer = {
		.speed_mode		 = config->speed_mode,
		.timer_num       = config->timer_num, 		
		.duty_resolution = config->duty_resolution,
		.freq_hz 		 = config->freq_hz,
		.clk_cfg 		 = LEDC_AUTO_CLK,//CPC405X_PWM_CLK_CFG,
	};
	ret = ledc_timer_config(&ledc_timer);
	if (ret != ESP_OK) return ret;

	ledc_channel_config_t ledc_channel = {
		.speed_mode = config->speed_mode,
		.timer_sel  = config->timer_num,
		.channel    = config->channel,
        .intr_type	= LEDC_INTR_DISABLE, //  固定输出不需要中断
		.gpio_num   = config->output_io_num,
		.duty       = 0, // 先初始化为0%
		.hpoint     = 0,
	};
    ret = ledc_channel_config(&ledc_channel);
	if (ret != ESP_OK) return ret;

	_ldo[config->ldo_num - 1].initialized = true;
	_ldo[config->ldo_num - 1].is_on = false;

    return ESP_OK;
}

esp_err_t CPC405x_LDO_On(uint32_t delay, CPC405x_LDO_num_t num) {
    ESP_ERROR_CHECK(num == CPC405x_LDO_None || num >= CPC405x_LDO_Max);
	esp_err_t ret = ledc_set_duty(
						_ldo[num - 1].config.speed_mode, 
						_ldo[num - 1].config.channel, 
						(_ldo[num - 1].config.duty_resolution >> 1) - 1 /* 50% */
						);
	if (ret != ESP_OK) return ret;

	ret = ledc_update_duty(_ldo[num - 1].config.speed_mode, _ldo[num - 1].config.channel);
	if (ret != ESP_OK) return ret;

	// 防止连续调用导致过度延时
	if (_ldo[num - 1].is_on == false) {
		_ldo[num - 1].is_on = true; 
		if (delay > 0) {
			vTaskDelay(delay / portTICK_RATE_MS);
		}
	}
	return ESP_OK;
}

esp_err_t CPC405x_LDO_Off(CPC405x_LDO_num_t num) {
    ESP_ERROR_CHECK(num == CPC405x_LDO_None || num >= CPC405x_LDO_Max);
	_ldo[num - 1].is_on = false;
	return ledc_stop(_ldo[num - 1].config.speed_mode, _ldo[num - 1].config.channel, 0/*设置为低电平*/);
}

#define LEDC_TIMER_DIV_NUM_MAX	(0x3FF)
#define LEDC_RTC_CLK_8M    		(8000000)
ledc_timer_bit_t LEDC_Duty_Resolution_Tool(CPC405x_LDO_Config_t *config) {
	uint32_t div_param = 0;
    uint32_t precision = ( 0x1 << config->duty_resolution );
    ledc_clk_src_t timer_clk_src = LEDC_APB_CLK;

	uint32_t s_ledc_slow_clk_8M = LEDC_RTC_CLK_8M ;

    // Calculate the divisor

    // 自动选择 APB 或 REF_TICK 作为时钟源。
    if (config->clk_cfg == LEDC_AUTO_CLK) {
		// 尝试根据 LEDC_APB_CLK 计算div_param
		div_param = LEDC_APB_CLK_HZ / config->freq_hz / precision;
		if (div_param > LEDC_TIMER_DIV_NUM_MAX) {
			// APB_CLK 导致div_param 过高。 尝试使用 REF_TICK 作为时钟源。
			timer_clk_src = LEDC_REF_TICK;
			div_param = LEDC_REF_CLK_HZ / config->freq_hz / precision;
		} else if (div_param < 1) {
			/* divisor 太小
			goto error;
			*/
		}   
    } 
	// 用户指定的低速通道时钟源(RTC8M_CLK)  
    else if ((config->speed_mode == LEDC_LOW_SPEED_MODE) && (config->clk_cfg == LEDC_USE_RTC8M_CLK)) {
        div_param = LEDC_RTC_CLK_8M / config->freq_hz / precision;

	// 用户指定的时钟源(LEDC_APB_CLK_HZ or LEDC_REF_TICK)
	} else {
		// timer_clk_src = (config->clk_cfg == LEDC_USE_REF_TICK) ? LEDC_REF_TICK : LEDC_APB_CLK;
		uint32_t src_clk_freq = (config->clk_cfg == LEDC_USE_REF_TICK) ? LEDC_REF_CLK_HZ : LEDC_APB_CLK_HZ;

		div_param = src_clk_freq / config->freq_hz / precision;
	}
    
    if (div_param < 1 || div_param > LEDC_TIMER_DIV_NUM_MAX) {
		/* 不在范围内
        goto error;
		*/
    }

	return 0;
}