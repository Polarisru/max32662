/**
 * @file sh1106.h
 * @brief SH1106 OLED display driver library (I2C, hardware-abstracted)
 *
 * Display resolution: 128x64 pixels
 * I2C interface, hardware functions provided via callback abstraction.
 *
 * Usage:
 *   1. Implement sh1106_i2c_write_fn (send bytes over I2C)
 *   2. Call sh1106_init() with your I2C write callback
 *   3. Use draw functions, then sh1106_update() to push to display
 */

#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================================
 * Configuration
 * ========================================================================= */

#define SH1106_WIDTH        128
#define SH1106_HEIGHT       64
#define SH1106_PAGES        (SH1106_HEIGHT / 8)   /* 8 pages of 8 rows each */

/* Default I2C address (SA0=GND -> 0x3C, SA0=VCC -> 0x3D) */
#define SH1106_I2C_ADDR     0x3C

/* =========================================================================
 * I2C Hardware Abstraction
 * ========================================================================= */

/**
 * @brief User-supplied I2C write function.
 *
 * @param addr   7-bit I2C device address
 * @param data   Pointer to bytes to transmit
 * @param len    Number of bytes
 * @return       0 on success, non-zero on error
 */
typedef int (*sh1106_i2c_write_fn)(uint8_t addr, const uint8_t *data, size_t len);

/* =========================================================================
 * Color / pixel value
 * ========================================================================= */

typedef enum {
    SH1106_BLACK  = 0,
    SH1106_WHITE  = 1,
    SH1106_INVERT = 2   /* XOR pixel with existing value */
} sh1106_color_t;

/* =========================================================================
 * Font descriptor
 * ========================================================================= */

/**
 * @brief Font descriptor pointing to a glyph bitmap table.
 *
 * Each glyph is stored as (width * height_pages) bytes, column-major,
 * LSB = top row of the page.
 *
 * Example: 5x8 font  ->  5 bytes per glyph (1 page tall)
 *          5x16 font -> 10 bytes per glyph (2 pages tall)
 */
typedef struct {
    const uint8_t *data;        /**< Raw bitmap data                        */
    uint8_t        first_char;  /**< ASCII code of first glyph              */
    uint8_t        last_char;   /**< ASCII code of last glyph (inclusive)   */
    uint8_t        width;       /**< Glyph width in pixels                  */
    uint8_t        height;      /**< Glyph height in pixels (multiple of 8) */
    uint8_t        spacing;     /**< Extra horizontal pixels between glyphs */
} sh1106_font_t;

/* =========================================================================
 * Driver handle
 * ========================================================================= */

typedef struct {
    sh1106_i2c_write_fn i2c_write;                        /**< Bound I2C write callback */
    uint8_t             i2c_addr;                         /**< Device I2C address       */
    uint8_t             framebuf[SH1106_PAGES][SH1106_WIDTH]; /**< Frame buffer         */
    bool                inverted;                         /**< Display inversion state  */
} sh1106_t;

/* =========================================================================
 * Init / control
 * ========================================================================= */

/**
 * @brief Initialise driver and configure the SH1106 hardware.
 *
 * @param dev        Driver handle to initialise
 * @param write_fn   Platform I2C write callback
 * @param i2c_addr   7-bit I2C address (use SH1106_I2C_ADDR for default)
 * @return           0 on success
 */
int  sh1106_init(sh1106_t *dev, sh1106_i2c_write_fn write_fn, uint8_t i2c_addr);

/** @brief Turn display on */
void sh1106_display_on(sh1106_t *dev);

/** @brief Turn display off (sleep) */
void sh1106_display_off(sh1106_t *dev);

/** @brief Invert all pixels on/off */
void sh1106_invert(sh1106_t *dev, bool invert);

/** @brief Set display contrast (0-255) */
void sh1106_set_contrast(sh1106_t *dev, uint8_t contrast);

/** @brief Flush the internal frame buffer to the display */
int  sh1106_update(sh1106_t *dev);

/* =========================================================================
 * Frame buffer operations
 * ========================================================================= */

/** @brief Clear the frame buffer (fill with BLACK) */
void sh1106_clear(sh1106_t *dev);

/** @brief Fill the entire frame buffer with color */
void sh1106_fill(sh1106_t *dev, sh1106_color_t color);

/** @brief Set a single pixel */
void sh1106_draw_pixel(sh1106_t *dev, int16_t x, int16_t y, sh1106_color_t color);

/** @brief Get pixel value from frame buffer (returns 0 or 1) */
uint8_t sh1106_get_pixel(const sh1106_t *dev, int16_t x, int16_t y);

/* =========================================================================
 * Primitive drawing
 * ========================================================================= */

/** @brief Draw a horizontal line */
void sh1106_draw_hline(sh1106_t *dev, int16_t x, int16_t y, int16_t w, sh1106_color_t color);

/** @brief Draw a vertical line */
void sh1106_draw_vline(sh1106_t *dev, int16_t x, int16_t y, int16_t h, sh1106_color_t color);

/** @brief Draw an arbitrary line (Bresenham) */
void sh1106_draw_line(sh1106_t *dev, int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, sh1106_color_t color);

/** @brief Draw a rectangle outline */
void sh1106_draw_rect(sh1106_t *dev, int16_t x, int16_t y,
                      int16_t w, int16_t h, sh1106_color_t color);

/** @brief Draw a filled rectangle */
void sh1106_fill_rect(sh1106_t *dev, int16_t x, int16_t y,
                      int16_t w, int16_t h, sh1106_color_t color);

/** @brief Draw a circle outline (Midpoint algorithm) */
void sh1106_draw_circle(sh1106_t *dev, int16_t cx, int16_t cy,
                        int16_t r, sh1106_color_t color);

/** @brief Draw a filled circle */
void sh1106_fill_circle(sh1106_t *dev, int16_t cx, int16_t cy,
                        int16_t r, sh1106_color_t color);

/** @brief Draw a triangle outline */
void sh1106_draw_triangle(sh1106_t *dev,
                          int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          sh1106_color_t color);

/** @brief Draw a filled triangle */
void sh1106_fill_triangle(sh1106_t *dev,
                          int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          sh1106_color_t color);

/**
 * @brief Draw a 1-bit bitmap (MSB first per byte, row-major).
 *
 * @param bitmap  Packed 1-bit image, ceil(w/8) bytes per row
 * @param color   SH1106_WHITE draws set bits, SH1106_BLACK clears them
 */
void sh1106_draw_bitmap(sh1106_t *dev, int16_t x, int16_t y,
                        const uint8_t *bitmap, int16_t w, int16_t h,
                        sh1106_color_t color);

/* =========================================================================
 * Text rendering
 * ========================================================================= */

/**
 * @brief Draw a single character.
 * @return Horizontal advance in pixels (width + spacing).
 */
int16_t sh1106_draw_char(sh1106_t *dev, int16_t x, int16_t y,
                         char c, const sh1106_font_t *font,
                         sh1106_color_t color);

/**
 * @brief Draw a null-terminated string.
 *        '\n' advances to x_start, y += font->height.
 */
void sh1106_draw_string(sh1106_t *dev, int16_t x, int16_t y,
                        const char *str, const sh1106_font_t *font,
                        sh1106_color_t color);

/** @brief Measure pixel width of a string without drawing. */
int16_t sh1106_string_width(const char *str, const sh1106_font_t *font);

#endif /* SH1106_H */
