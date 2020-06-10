/**
 * @file st7735.c
 *
 * Mostly taken from lbthomsen/esp-idf-littlevgl github.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "st7735.h"

#include "disp_spi.h"
#include "driver/gpio.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[17];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void st7735_send_cmd(uint8_t cmd);

static void st7735_send_data(void *data, uint16_t length);

static void st7735_send_color(void *data, uint16_t length);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void st7735_init_base(lcd_init_cmd_t st7735_init_cmds[]) {
    //Initialize non-SPI GPIOs
    gpio_set_direction(ST7735_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(ST7735_RST, GPIO_MODE_OUTPUT);

#if ST7735_ENABLE_BACKLIGHT_CONTROL
    gpio_set_direction(ST7735_BCKL, GPIO_MODE_OUTPUT);
#endif

    //Reset the display
    gpio_set_level(ST7735_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(ST7735_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    printf("ST7735 initialization.\n");

    //Send all the commands
    uint16_t cmd = 0;
    while (st7735_init_cmds[cmd].databytes != 0xff) {

        uint16_t len = st7735_init_cmds[cmd].databytes & 0x1F;
        uint16_t hasDelay = st7735_init_cmds[cmd].databytes & 0x80;

        st7735_send_cmd(st7735_init_cmds[cmd].cmd);
        st7735_send_data(st7735_init_cmds[cmd].data, len);

        if (hasDelay) {
            vTaskDelay(st7735_init_cmds[cmd].data[len + 1] / portTICK_RATE_MS);
        }

        cmd++;
    }

    printf("processed %i cmds.\n", cmd);
    printf("ST7735 initialization complete.\n");

    st7735_enable_backlight(true);
}

#if ST7735_INITB
void st7735_init() {
    lcd_init_cmd_t st7735_init_cmds[] = {
            {ST7735_SWRESET, {50}, ST7735_INIT_DELAY},      //  1: Software reset, no args, w/50ms delay
            {ST7735_SLPOUT, {255}, ST7735_INIT_DELAY},      //  2: Out of sleep mode, no args, w/500ms(?) delay
            {ST7735_COLMOD, {                                      //  3: Set color mode
                   0x05,                                   //     16-bit color
                    10,                                     //     10 ms delay
            }, 1 + ST7735_INIT_DELAY},                    //     1 arg + delay
            {ST7735_FRMCTR1, {                                     //  4: Frame rate control
                    0x00,                                   //     fastest refresh
                    0x06,                                   //     6 lines front porch
                    0x03,                                   //     3 lines back porch
                    10,                                     //     10 ms delay
            }, 3 + ST7735_INIT_DELAY},                    //     3 args + delay
            { ST7735_MADCTL, {                                     //  5: Memory access ctrl (directions)
                    0x40,                                   //     Row addr/col addr, bottom to top refresh
            }, 1},
            { ST7735_DISSET5,{                                     //  6: Display settings #5,
                     0x15,                                  //     1 clk cycle nonoverlap, 2 cycle gate
                                                                   //     rise, 3 cycle osc equalize
                     0x02,                                  //     Fix on VTL
            }, 2},
            { ST7735_INVCTR, {                                     //  7: Display inversion control
                     0x0                                    //     Line inversion
            },  1},
            { ST7735_PWCTR1,{                                      //  8: Power control
                     0x02,                                  //     GVDD = 4.7V
                     0x70,                                  //     1.0uA
                     10,                                    //     10 ms delay
            }, 2 + ST7735_INIT_DELAY},
            { ST7735_PWCTR2, {                                     //  9: Power control
                    0x05                                    //     VGH = 14.7V, VGL = -7.35V
            },  1},
            { ST7735_PWCTR3, {                                     //  10: Power control
                     0x01,                                  //     Opamp current small
                     0x02,                                  //     Boost frequency
            },  2},
            { ST7735_VMCTR1, {                                     //  11: Power control
                     0x3C,                                  //     VCOMH = 4V
                     0x38,                                  //     VCOML = -1.1V
                     10,                                    //     10 ms delay
            },  2 + ST7735_INIT_DELAY},
            { ST7735_PWCTR6, {                                     //  12: Power control
                     0x11,
                     0x15,
            },  2},
            { ST7735_GMCTRP1, {                                    //  13: Magical unicorn dust
                      0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
                      0x21, 0x1B, 0x13, 0x19, //      these config values represent)
                      0x17, 0x15, 0x1E, 0x2B,
                      0x04, 0x05, 0x02, 0x0E,
            },  16},
            { ST7735_GMCTRN1, {                                    //  14: Sparkles and rainbows
                      0x0B, 0x14, 0x08, 0x1E, //     (ditto)
                      0x22, 0x1D, 0x18, 0x1E,
                      0x1B, 0x1A, 0x24, 0x2B,
                      0x06, 0x06, 0x02, 0x0F,
                      10,                                  //     10 ms delay
            },  16 + ST7735_INIT_DELAY},
            { ST7735_CASET, {                                      //  15: Column addr set
                        0x00, 0x02,                  //     XSTART = 2
                        0x00, 0x81,                  //     XEND = 129
            },  4},
            { ST7735_RASET, {                                      //  16: Row addr set
                        0x00, 0x02,                  //     XSTART = 1
                        0x00, 0x81,                  //     XEND = 160
            },  4},
            { ST7735_NORON, {                                      //  17: Normal display on
                        10,                                 //     10 ms delay
            },  ST7735_INIT_DELAY},
            { ST7735_DISPON, {                                     //  18: Main screen turn on
                    255,                                    //     255 = 500 ms delay
            },  ST7735_INIT_DELAY},
            {0xff, {}, 0xff}
    };

    st7735_init_base(st7735_init_cmds);
}
#elif ST7735_INITR_GREENTAB160x80
void st7735_init() {
    lcd_init_cmd_t st7735_init_cmds[] = {
            {ST7735_SWRESET, {150}, ST7735_INIT_DELAY},  //  1: Software reset, no args, w/150ms delay
            {ST7735_SLPOUT,  {255}, ST7735_INIT_DELAY},   //  2: Out of sleep mode, no args, w/500ms(?) delay
            {ST7735_FRMCTR1, {                                  //  3: Frame rate ctrl - normal mode
                              0x01, 0x2C, 0x2D,                                   //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
                             },  3},
            {ST7735_FRMCTR2, {                                  //  4: Frame rate control - idle mode
                              0x01, 0x2C, 0x2D,                                   //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
                             },  3},
            {ST7735_FRMCTR3, {                                  //  5: Frame rate ctrl - partial mode
                              0x01, 0x2C, 0x2D,                                   //     Dot inversion mode
                                                0x01, 0x2C, 0x2D,                                   //     Line inversion mode
                             },  6},
            {ST7735_INVCTR,  {                                   //  6: Display inversion ctrl
                              0x07,                                           //     No inversion
                             },  1},
            {ST7735_PWCTR1,  {                                   //  7: Power control
                              0xA2,
                                    0x02,                                           //     -4.6V
                                          0x84,                                           //     AUTO mode
                             },  3},
            {ST7735_PWCTR2,  {                                   //  8: Power control
                              0xC5,                                           //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
                             },  1},
            {ST7735_PWCTR3,  {                                   //  9: Power control
                              0x0A,                                           //     Opamp current small
                                    0x00,                                           //     Boost frequency
                             },  2},
            {ST7735_PWCTR4,  {                                   // 10: Power control
                              0x8A,                                           //     BCLK/2, Opamp current small & Medium low
                                    0x2A,
                             },  2},
            {ST7735_PWCTR5,  {                                   // 11: Power control
                              0x8A, 0xEE,
                             },  2},
            {ST7735_VMCTR1,  {                                   // 12: Power control
                              0x0E,
                             },  1},
            {ST7735_INVOFF,  {                                   // 13: Don't invert display
                             },  0},
            {ST7735_MADCTL,  {                                   // 14: Memory access control (directions)
                              0xC8,                                           //     row addr/col addr, bottom to top refresh
                             },  1},
            {ST7735_COLMOD,  {                                   // 15: set color mode
                              0x05,                                           //     16-bit color
                             },  1},
            {ST7735_CASET,   {
                              0x00, 0x02,                                     //     XSTART = 0
                                          0x00, 0x7F + 0x02,                                //     XEND = 127
                             },  4},
            {ST7735_RASET,   {
                              0x00, 0x01,                                      //     XSTART = 0
                                          0x00, 0x9F + 0x01                                  //     XEND = 159
                             },  4},
            {ST7735_INVON,   {}, 0},
            {ST7735_GMCTRP1, {
                              0x02, 0x1c, 0x07, 0x12,
                                                      0x37, 0x32, 0x29, 0x2d,
                                     0x29, 0x25, 0x2B, 0x39,
                                     0x00, 0x01, 0x03, 0x10,
                             },  16},
            {ST7735_GMCTRN1, {
                              0x03, 0x1d, 0x07, 0x06,
                                                      0x2E, 0x2C, 0x29, 0x2D,
                                     0x2E, 0x2E, 0x37, 0x3F,
                                     0x00, 0x00, 0x02, 0x10,
                             },  16},
            {ST7735_NORON,   {
                              10
                             },     ST7735_INIT_DELAY},
            {ST7735_DISPON,  {
                              100
                             },     ST7735_INIT_DELAY},
            {0xff,           {}, 0xff}
    };

    st7735_init_base(st7735_init_cmds);
}
#endif

void st7735_enable_backlight(bool backlight) {
#if ST7735_ENABLE_BACKLIGHT_CONTROL
    printf("%s backlight.\n", backlight ? "Enabling" : "Disabling");
    uint32_t tmp = 0;

#if (ST7735_BCKL_ACTIVE_LVL == 1)
    tmp = backlight ? 1 : 0;
#else
    tmp = backlight ? 0 : 1;
#endif

    gpio_set_level(ST7735_BCKL, tmp);
#endif
}


void st7735_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    uint8_t data[4] = {0};

    /*Column addresses*/
    st7735_send_cmd(ST7735_CASET);
    data[0] = ((area->x1 + X_OFFSET) >> 8) & 0xFF;
    data[1] = (area->x1 + X_OFFSET) & 0xFF;
    data[2] = ((area->x2 + X_OFFSET) >> 8) & 0xFF;
    data[3] = (area->x2 + X_OFFSET) & 0xFF;
    st7735_send_data(data, 4);

    /*Page addresses*/
    st7735_send_cmd(ST7735_RASET);
    data[0] = ((area->y1 + Y_OFFSET) >> 8) & 0xFF;
    data[1] = (area->y1 + Y_OFFSET) & 0xFF;
    data[2] = ((area->y2 + Y_OFFSET) >> 8) & 0xFF;
    data[3] = (area->y2 + Y_OFFSET) & 0xFF;
    st7735_send_data(data, 4);

    /*Memory write*/
    st7735_send_cmd(ST7735_RAMWR);

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    /*Byte swapping is required*/
    uint32_t i;
    uint8_t * color_u8 = (uint8_t *) color_map;
    uint8_t color_tmp;
    for(i = 0; i < size * 2; i += 2) {
        color_tmp = color_u8[i + 1];
        color_u8[i + 1] = color_u8[i];
        color_u8[i] = color_tmp;
    }


    while(size > ST7735_DISPLAY_WIDTH) {

        st7735_send_color((void*)color_map, ST7735_DISPLAY_WIDTH * 2);
        //vTaskDelay(10 / portTICK_PERIOD_MS);
        size -= ST7735_DISPLAY_WIDTH;
        color_map += ST7735_DISPLAY_WIDTH;
    }

    st7735_send_color((void *) color_map, size * 2);

    lv_flush_ready();
}

//static void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
//{
//    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
//
//    int32_t len = (x2-x1+1)*(y2-y1+1);
//    if(len >= 32*128){
//        st7735_Address_set(x1,y1,x1+x2,y1+y2);
//        while(len--)
//        {
//            st7735_writedata16(color_p->full);
//            color_p++;
//        }
//    }else{
//        int32_t x;
//        int32_t y;
//        for(y = y1; y <= y2; y++) {
//            for(x = x1; x <= x2; x++) {
//                /* Put a pixel to the display. For example: */
//                /* put_px(x, y, *color_p)*/
//                LCD_DrawPoint_set(x,y,color_p->full);
//                color_p++;
//            }
//        }
//    }
//
//    //printf("flush\n");
//    /* IMPORTANT!!!
//     * Inform the graphics library that you are ready with the flushing*/
//    lv_flush_ready();
//}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void st7735_send_cmd(uint8_t cmd) {
    disp_wait_for_pending_transactions();
    gpio_set_level(ST7735_DC, 0);
    disp_spi_send_data(&cmd, 1);
}

static void st7735_send_data(void *data, uint16_t length) {
    disp_wait_for_pending_transactions();
    gpio_set_level(ST7735_DC, 1);
    disp_spi_send_data(data, length);
}

static void st7735_send_color(void *data, uint16_t length) {
    disp_wait_for_pending_transactions();
    gpio_set_level(ST7735_DC, 1);
    disp_spi_send_colors(data, length);
}
