#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_sleep.h"

#define LED_G_RTC_BIT (RTC_GPIO_OUT_DATA_S + 6)
#define LED_R_RTC_BIT (RTC_GPIO_OUT_DATA_S + 7)

#define CHRG_RTC_BIT (RTC_GPIO_IN_NEXT_S + 9)
#define STDBY_RTC_BIT (RTC_GPIO_IN_NEXT_S + 10)

#define LED_G_EN_BIT (RTC_GPIO_ENABLE_W1TS_S + 6)
#define LED_R_EN_BIT (RTC_GPIO_ENABLE_W1TS_S + 7)

#define DELAY 10
#define PWM_OFF_REPS 9

void ulp()
{
  rtc_gpio_init(GPIO_NUM_25);
  rtc_gpio_set_direction(GPIO_NUM_25, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_pullup_dis(GPIO_NUM_25);
  rtc_gpio_pulldown_dis(GPIO_NUM_25);
  rtc_gpio_set_level(GPIO_NUM_25, 0);
  rtc_gpio_hold_dis(GPIO_NUM_25);

  rtc_gpio_init(GPIO_NUM_26);
  rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_pullup_dis(GPIO_NUM_26);
  rtc_gpio_pulldown_dis(GPIO_NUM_26);
  rtc_gpio_set_level(GPIO_NUM_26, 0);
  rtc_gpio_hold_dis(GPIO_NUM_26);

  rtc_gpio_init(GPIO_NUM_32);
  rtc_gpio_set_direction(GPIO_NUM_32, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en(GPIO_NUM_32);
  rtc_gpio_pulldown_dis(GPIO_NUM_32);
  rtc_gpio_hold_en(GPIO_NUM_32);

  rtc_gpio_init(GPIO_NUM_4);
  rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en(GPIO_NUM_4);
  rtc_gpio_pulldown_dis(GPIO_NUM_4);
  rtc_gpio_hold_en(GPIO_NUM_4);

  const ulp_insn_t ulp_prog[] = {
      I_WR_REG_BIT(RTC_GPIO_ENABLE_W1TS_REG, LED_G_EN_BIT, 1),
      I_WR_REG_BIT(RTC_GPIO_ENABLE_W1TS_REG, LED_R_EN_BIT, 1),

      M_LABEL(1),
      I_RD_REG(RTC_GPIO_IN_REG, CHRG_RTC_BIT, CHRG_RTC_BIT),
      I_ANDI(R0, R0, 1),
      M_BXZ(2),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_R_RTC_BIT, 0),
      M_BX(3),

      M_LABEL(2),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_R_RTC_BIT, 1),
      I_DELAY(DELAY),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_R_RTC_BIT, 0),
      I_STAGE_RST(),
      M_LABEL(10),
      I_DELAY(DELAY),
      I_STAGE_INC(1),
      M_BSLT(10, PWM_OFF_REPS),

      M_LABEL(3),
      I_RD_REG(RTC_GPIO_IN_REG, STDBY_RTC_BIT, STDBY_RTC_BIT),
      I_ANDI(R0, R0, 1),
      M_BXZ(4),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_G_RTC_BIT, 0),
      M_BX(5),

      M_LABEL(4),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_G_RTC_BIT, 1),
      I_DELAY(DELAY),
      I_WR_REG_BIT(RTC_GPIO_OUT_REG, LED_G_RTC_BIT, 0),
      I_STAGE_RST(),
      M_LABEL(11),
      I_DELAY(DELAY),
      I_STAGE_INC(1),
      M_BSLT(11, PWM_OFF_REPS),

      M_LABEL(5),
      I_DELAY(DELAY),
      M_BX(1),
  };

  size_t size = sizeof(ulp_prog) / sizeof(ulp_insn_t);
  ESP_ERROR_CHECK_WITHOUT_ABORT(ulp_process_macros_and_load(0, ulp_prog, &size));
  ESP_ERROR_CHECK_WITHOUT_ABORT(ulp_run(0));
}