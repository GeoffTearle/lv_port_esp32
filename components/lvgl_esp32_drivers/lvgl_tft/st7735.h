/**
 * @file st7735.h
 *
 * Mostly taken from lbthomsen/esp-idf-littlevgl github.
 */

#ifndef ST7735_H
#define ST7735_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl/lvgl.h"
#include "sdkconfig.h"

#define DISP_BUF_SIZE   (LV_HOR_RES_MAX * 40)
#define ST7735_DC       CONFIG_LVGL_DISP_PIN_DC
#define ST7735_RST      CONFIG_LVGL_DISP_PIN_RST
#define ST7735_BCKL     CONFIG_LVGL_DISP_PIN_BCKL

#define ST7735_DISPLAY_WIDTH CONFIG_LVGL_DISPLAY_WIDTH
#define ST7735_DISPLAY_HEIGHT CONFIG_LVGL_DISPLAY_HEIGHT

#define ST7735_ENABLE_BACKLIGHT_CONTROL CONFIG_LVGL_ENABLE_BACKLIGHT_CONTROL
#define ST7735_INITR_GREENTAB160x80 CONFIG_LVGL_TFT_DISPLAY_CONTROLLER_ST7735_GREENTAB160x80

#if CONFIG_LVGL_BACKLIGHT_ACTIVE_LVL
  #define ST7735_BCKL_ACTIVE_LVL 1
#else
  #define ST7735_BCKL_ACTIVE_LVL 0
#endif

#ifdef ST7735_INTR_GREENTAB
#define X_OFFSET 2
#define Y_OFFSET 1
#elif ST7735_INTR_GREENTAB2
#define X_OFFSET 2
#define Y_OFFSET 1
#elif ST7735_INTR_GREENTAB3
#define X_OFFSET 2
#define Y_OFFSET 3
#elif ST7735_INTR_GREENTAB128
#define X_OFFSET 0
#define Y_OFFSET 32
#elif ST7735_INITR_GREENTAB160x80
#define X_OFFSET 26
#define Y_OFFSET 1
#elif ST7735_INITR_REDTAB160x80
#define X_OFFSET 24
#define Y_OFFSET 0
#elif ST7735_INITR_REDTAB
#define X_OFFSET 0
#define Y_OFFSET 0
#elif ST7735_INITR_BLACKTAB
#define X_OFFSET 0
#define Y_OFFSET 0
#else
#define X_OFFSET 0
#define Y_OFFSET 0
#endif

/* ST7735 commands */
// ST7735 specific commands used in init
#define ST7735_INIT_DELAY 0x80

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B // PASET
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

void st7735_init(void);
void st7735_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void st7735_enable_backlight(bool backlight);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ST7735_H  */
