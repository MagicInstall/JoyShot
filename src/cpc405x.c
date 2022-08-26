/**
 *  CPC405x 电源管理驱动
 *  
 *  2022-08-18    wing    创建.
 */

#include <string.h>
#include "cpc405x.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "CPC405X";

CPC405x_Battery_Config_t   	_battery_config;
QueueHandle_t 				_charge_ent_queue 	= NULL;


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
	// ESP_LOGI(TAG, "The maximum ledc timer bit available is LEDC_TIMER_%d_BIT", 
	// 		 LEDC_Duty_Resolution_Tool(config)); // 测试用
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
	if (_ldo[num - 1].initialized == false) return ESP_FAIL;

	esp_err_t ret = ledc_set_duty(
						_ldo[num - 1].config.speed_mode, 
						_ldo[num - 1].config.channel, 
						(1 << _ldo[num - 1].config.duty_resolution) / 2 /* 50% */
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

#define LEDC_TIMER_DUTY_MAX		((0x2 << SOC_LEDC_TIMER_BIT_WIDE_NUM) - 1)
#define LEDC_RTC_CLK_8M    		(8000000)
ledc_timer_bit_t LEDC_Duty_Resolution_Tool(CPC405x_LDO_Config_t *config) {
	uint32_t div_param = 0;

    // 自动选择 APB 或 REF_TICK 作为时钟源。
    if (config->clk_cfg == LEDC_AUTO_CLK) {
		// 尝试根据 LEDC_APB_CLK 计算div_param
		div_param = LEDC_APB_CLK_HZ / config->freq_hz;
		if (div_param > LEDC_TIMER_DUTY_MAX) {
			// APB_CLK 导致div_param 过高。 尝试使用 REF_TICK 作为时钟源。
			div_param = LEDC_REF_CLK_HZ / config->freq_hz;
		}  
    } 
	// 用户指定的低速通道时钟源(RTC8M_CLK)  
    else if ((config->speed_mode == LEDC_LOW_SPEED_MODE) && (config->clk_cfg == LEDC_USE_RTC8M_CLK)) {
        div_param = LEDC_RTC_CLK_8M / config->freq_hz;

	} else {
		// timer_clk_src = (config->clk_cfg == LEDC_USE_REF_TICK) ? LEDC_REF_TICK : LEDC_APB_CLK;
		uint32_t src_clk_freq = (config->clk_cfg == LEDC_USE_REF_TICK) ? LEDC_REF_CLK_HZ : LEDC_APB_CLK_HZ;

		div_param = src_clk_freq / config->freq_hz;
	}

    // 不在范围内
    if (div_param < 1 || div_param > LEDC_TIMER_DUTY_MAX) return 0;
    
	uint8_t bits = 0;
	while (div_param > 1)
	{
		div_param >>= 1;
		bits++;
	}

	return bits;
}


/// 插拔充电器中断
static void IRAM_ATTR _charge_isr_handler(void* arg) 
{
	xQueueOverwriteFromISR(_charge_ent_queue, NULL, NULL);
}

static void _charge_task(void *param) {
	ESP_ERROR_CHECK(param == NULL);
	// CPC405x_Battery_Config_t *config = (CPC405x_Battery_Config_t *)param;
	uint8_t ent_val, last_val = 2/*让第一次判断ent_val必定不等于*/;
	BaseType_t res;
	while (1)
	{
		res = xQueueReceive(_charge_ent_queue, &ent_val, portMAX_DELAY);
		if (res == pdTRUE) {
			// 先跳过1 Tick的时间当作消抖
			vTaskDelay(1);
			ent_val = gpio_get_level(_battery_config.status_io_num) == 0 ? 
							CPC405X_CHARGE_EVENT_CHARGING : 
							CPC405X_CHARGE_EVENT_UNCHARG;

			// 防止重复触发同一个事件
			if (ent_val != last_val) {
				last_val = ent_val;
				ESP_LOGI(TAG, "%s event",   ent_val == CPC405X_CHARGE_EVENT_CHARGING?
											"Charging" :
											"Uncharge");
				_battery_config.charge_cb(ent_val);
			}
		}
		// TODO：测试用
		else {
			ESP_LOGI(TAG, "_charge_task: xQueueReceive skip once.");
		}
	}
	
}

// 滤波采用平均值算法（实测连续几次采样的值都相差很小	wing），
// 采样次数使用2的N次幂是为了避免使用除法，
// 直接位移就能求出平均数。
static void _adc_task(void *param) {
	ESP_ERROR_CHECK(param == NULL);
	static const TickType_t per_min = 60 * 1000 / portTICK_RATE_MS;
	CPC405x_Battery_Config_t *config = (CPC405x_Battery_Config_t *)param;
	ESP_ERROR_CHECK(config->level_cb == NULL);

	int val, times = 1 << BATTERY_ADC_FILTER_POWER, sum[times];
	static int last = 0;

	while (1) {	
		val = 0;
		for (size_t i = 0; i < times; i++)
		{
			if (config->adc_unit == ADC_UNIT_1) {
				sum[i] = adc1_get_raw(config->adc_channel);
			}
			else if (config->adc_unit == ADC_UNIT_2) {
				ESP_ERROR_CHECK(adc2_get_raw(config->adc_channel, BATTERY_ADC_WIDTH, sum + i));
			}
			else {
				ESP_LOGW(TAG, "Unknow unit!");
			}
			val += *(sum + i);
			// ESP_LOGI(TAG, "ADC test: %d", sum[i]); // 测试用
		}
		val >>= BATTERY_ADC_FILTER_POWER;

		ESP_LOGI(TAG, "ADC: %d", val);
		if (val != last) config->level_cb(val);
		last = val;
		vTaskDelay(config->adc_interval * per_min);
		// vTaskDelay(5000 / portTICK_RATE_MS); // 测试用
	}
}

esp_err_t CPC405x_Battery_Init(CPC405x_Battery_Config_t *config) {
	ESP_LOGI(TAG, "Battery Init");

	// 充电状态
	ESP_ERROR_CHECK(config == NULL);
	gpio_config_t io_config = {
        .pin_bit_mask   = (1ULL << config->status_io_num),
        .intr_type      = GPIO_INTR_ANYEDGE, // 上下降沿都触发
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,  
    };
    ESP_ERROR_CHECK(gpio_config(&io_config));

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
	esp_err_t rst = gpio_set_intr_type(config->status_io_num, 
    								   io_config.intr_type);
    if (rst != ESP_OK) return rst;

	_charge_ent_queue = xQueueCreate(1, 0);
	if (_charge_ent_queue == NULL) return ESP_ERR_NO_MEM;
	xQueueReset(_charge_ent_queue);

	rst = gpio_isr_handler_add(config->status_io_num, _charge_isr_handler, config);
    if (rst != ESP_OK) return rst;

	// 电量ADC
	if (config->adc_unit == ADC_UNIT_1) {
		rst = adc1_config_width(BATTERY_ADC_WIDTH);
    	if (rst != ESP_OK) return rst;

		rst = adc1_config_channel_atten(config->adc_channel, BATTERY_ADC_ATTEN);
	} 
	else if (config->adc_unit == ADC_UNIT_2) {
		rst = adc2_config_channel_atten(config->adc_channel, BATTERY_ADC_ATTEN);
	}
	else {
		return ESP_ERR_INVALID_ARG;
	}
    if (rst != ESP_OK) return rst;

	memcpy(&_battery_config, config, sizeof(CPC405x_Battery_Config_t));

	// adc 线程会立即触发一次电量事件
	xTaskCreate(_adc_task, "BATT_ADC", 2048, &_battery_config, 10, NULL);
	// 先触发一次充电器事件
	xQueueOverwrite(_charge_ent_queue, NULL);
	xTaskCreate(_charge_task, "CHARGE_TASK", 2048, &_battery_config, 10, NULL);

	return ESP_OK;
}
