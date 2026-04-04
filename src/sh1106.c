/**
 * @file sh1106.c
 * @brief SH1106 OLED display driver implementation
 */

#include "sh1106.h"
#include <string.h>
#include <stdlib.h>   /* abs() */

/* =========================================================================
 * Internal I2C helpers
 * ========================================================================= */

#define SH1106_CMD_BYTE   0x00   /* Co=0, D/C#=0 -> command stream */
#define SH1106_DATA_BYTE  0x40   /* Co=0, D/C#=1 -> data stream    */

static int _send_cmd(sh1106_t *dev, uint8_t cmd)
{
    uint8_t buf[2] = { SH1106_CMD_BYTE, cmd };
    return dev->i2c_write(dev->i2c_addr, buf, 2);
}

static int _send_cmd2(sh1106_t *dev, uint8_t cmd, uint8_t arg)
{
    uint8_t buf[3] = { SH1106_CMD_BYTE, cmd, arg };
    return dev->i2c_write(dev->i2c_addr, buf, 3);
}

/* =========================================================================
 * Initialisation
 * ========================================================================= */

int sh1106_init(sh1106_t *dev, sh1106_i2c_write_fn write_fn, uint8_t i2c_addr)
{
    if (!dev || !write_fn) return -1;

    dev->i2c_write = write_fn;
    dev->i2c_addr  = i2c_addr;
    dev->inverted  = false;
    memset(dev->framebuf, 0, sizeof(dev->framebuf));

    _send_cmd (dev, 0xAE);           /* Display OFF                     */
    _send_cmd2(dev, 0xD5, 0x80);     /* Clock divide / oscillator freq  */
    _send_cmd2(dev, 0xA8, 0x3F);     /* Multiplex ratio: 64             */
    _send_cmd2(dev, 0xD3, 0x00);     /* Display offset: 0               */
    _send_cmd (dev, 0x40);           /* Display start line: 0           */
    _send_cmd2(dev, 0xAD, 0x8B);     /* DC-DC on                        */
    _send_cmd (dev, 0xA1);           /* Segment remap: col127 -> SEG0   */
    _send_cmd (dev, 0xC8);           /* COM scan: remapped              */
    _send_cmd2(dev, 0xDA, 0x12);     /* COM pins hardware config        */
    _send_cmd2(dev, 0x81, 0x80);     /* Contrast: 128                   */
    _send_cmd2(dev, 0xD9, 0x1F);     /* Pre-charge period               */
    _send_cmd2(dev, 0xDB, 0x40);     /* VCOMH deselect level            */
    _send_cmd (dev, 0xA4);           /* Entire display on: follow RAM   */
    _send_cmd (dev, 0xA6);           /* Normal (non-inverted)           */
    _send_cmd (dev, 0xAF);           /* Display ON                      */

    return 0;
}

/* =========================================================================
 * Display control
 * ========================================================================= */

void sh1106_display_on (sh1106_t *dev) { _send_cmd(dev, 0xAF); }
void sh1106_display_off(sh1106_t *dev) { _send_cmd(dev, 0xAE); }

void sh1106_invert(sh1106_t *dev, bool invert)
{
    dev->inverted = invert;
    _send_cmd(dev, invert ? 0xA7 : 0xA6);
}

void sh1106_set_contrast(sh1106_t *dev, uint8_t contrast)
{
    _send_cmd2(dev, 0x81, contrast);
}

/* =========================================================================
 * Frame buffer flush
 *
 * SH1106 internal RAM is 132 columns wide; the visible 128-pixel area
 * starts at column 2, so we set the lower column address to 0x02.
 * ========================================================================= */

int sh1106_update(sh1106_t *dev)
{
    uint8_t buf[SH1106_WIDTH + 1];
    buf[0] = SH1106_DATA_BYTE;

    for (uint8_t page = 0; page < SH1106_PAGES; page++) {
        _send_cmd(dev, 0xB0 | page);   /* Set page address              */
        _send_cmd(dev, 0x02);          /* Lower column address (offset) */
        _send_cmd(dev, 0x10);          /* Higher column address = 0     */

        memcpy(&buf[1], dev->framebuf[page], SH1106_WIDTH);
        int ret = dev->i2c_write(dev->i2c_addr, buf, SH1106_WIDTH + 1);
        if (ret != 0) return ret;
    }
    return 0;
}

/* =========================================================================
 * Frame buffer helpers
 * ========================================================================= */

void sh1106_clear(sh1106_t *dev)
{
    memset(dev->framebuf, 0, sizeof(dev->framebuf));
}

void sh1106_fill(sh1106_t *dev, sh1106_color_t color)
{
    uint8_t val = (color == SH1106_WHITE) ? 0xFF : 0x00;
    memset(dev->framebuf, val, sizeof(dev->framebuf));
}

void sh1106_draw_pixel(sh1106_t *dev, int16_t x, int16_t y, sh1106_color_t color)
{
    if (x < 0 || x >= SH1106_WIDTH || y < 0 || y >= SH1106_HEIGHT) return;

    uint8_t *byte = &dev->framebuf[y / 8][x];
    uint8_t  bit  = (uint8_t)(1u << (y % 8));

    switch (color) {
        case SH1106_WHITE:  *byte |=  bit; break;
        case SH1106_BLACK:  *byte &= ~bit; break;
        case SH1106_INVERT: *byte ^=  bit; break;
    }
}

uint8_t sh1106_get_pixel(const sh1106_t *dev, int16_t x, int16_t y)
{
    if (x < 0 || x >= SH1106_WIDTH || y < 0 || y >= SH1106_HEIGHT) return 0;
    return (dev->framebuf[y / 8][x] >> (y % 8)) & 1u;
}

/* =========================================================================
 * Primitive drawing
 * ========================================================================= */

void sh1106_draw_hline(sh1106_t *dev, int16_t x, int16_t y,
                       int16_t w, sh1106_color_t color)
{
    for (int16_t i = x; i < x + w; i++)
        sh1106_draw_pixel(dev, i, y, color);
}

void sh1106_draw_vline(sh1106_t *dev, int16_t x, int16_t y,
                       int16_t h, sh1106_color_t color)
{
    for (int16_t i = y; i < y + h; i++)
        sh1106_draw_pixel(dev, x, i, color);
}

void sh1106_draw_line(sh1106_t *dev, int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, sh1106_color_t color)
{
    int16_t dx =  abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        sh1106_draw_pixel(dev, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void sh1106_draw_rect(sh1106_t *dev, int16_t x, int16_t y,
                      int16_t w, int16_t h, sh1106_color_t color)
{
    sh1106_draw_hline(dev, x,         y,         w, color);
    sh1106_draw_hline(dev, x,         y + h - 1, w, color);
    sh1106_draw_vline(dev, x,         y,         h, color);
    sh1106_draw_vline(dev, x + w - 1, y,         h, color);
}

void sh1106_fill_rect(sh1106_t *dev, int16_t x, int16_t y,
                      int16_t w, int16_t h, sh1106_color_t color)
{
    for (int16_t row = y; row < y + h; row++)
        sh1106_draw_hline(dev, x, row, w, color);
}

/* --- Circle (Midpoint algorithm) --- */

static void _circle_points(sh1106_t *dev, int16_t cx, int16_t cy,
                            int16_t px, int16_t py, sh1106_color_t color)
{
    sh1106_draw_pixel(dev, cx + px, cy + py, color);
    sh1106_draw_pixel(dev, cx - px, cy + py, color);
    sh1106_draw_pixel(dev, cx + px, cy - py, color);
    sh1106_draw_pixel(dev, cx - px, cy - py, color);
    sh1106_draw_pixel(dev, cx + py, cy + px, color);
    sh1106_draw_pixel(dev, cx - py, cy + px, color);
    sh1106_draw_pixel(dev, cx + py, cy - px, color);
    sh1106_draw_pixel(dev, cx - py, cy - px, color);
}

void sh1106_draw_circle(sh1106_t *dev, int16_t cx, int16_t cy,
                        int16_t r, sh1106_color_t color)
{
    int16_t px = 0, py = r, d = 1 - r;
    _circle_points(dev, cx, cy, px, py, color);
    while (px < py) {
        d += (d < 0) ? (2 * px + 3) : (2 * (px - py) + 5);
        if (d >= 0) py--;
        px++;
        _circle_points(dev, cx, cy, px, py, color);
    }
}

void sh1106_fill_circle(sh1106_t *dev, int16_t cx, int16_t cy,
                        int16_t r, sh1106_color_t color)
{
    int16_t px = 0, py = r, d = 1 - r;
    sh1106_draw_hline(dev, cx - r, cy, 2 * r + 1, color);
    while (px < py) {
        d += (d < 0) ? (2 * px + 3) : (2 * (px - py) + 5);
        if (d >= 0) py--;
        px++;
        sh1106_draw_hline(dev, cx - py, cy + px, 2 * py + 1, color);
        sh1106_draw_hline(dev, cx - py, cy - px, 2 * py + 1, color);
        sh1106_draw_hline(dev, cx - px, cy + py, 2 * px + 1, color);
        sh1106_draw_hline(dev, cx - px, cy - py, 2 * px + 1, color);
    }
}

/* --- Triangle --- */

void sh1106_draw_triangle(sh1106_t *dev,
                          int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          sh1106_color_t color)
{
    sh1106_draw_line(dev, x0, y0, x1, y1, color);
    sh1106_draw_line(dev, x1, y1, x2, y2, color);
    sh1106_draw_line(dev, x2, y2, x0, y0, color);
}

static void _tri_fill_flat(sh1106_t *dev,
                           int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           sh1106_color_t color)
{
    /* x0,y0 is the unique vertex; y1 == y2 is the flat edge */
    float inv1 = (float)(x1 - x0) / (y1 - y0);
    float inv2 = (float)(x2 - x0) / (y2 - y0);
    float ax = x0, bx = x0;
    int16_t step = (y1 >= y0) ? 1 : -1;

    for (int16_t y = y0; y != y1 + step; y += step) {
        int16_t xa = (int16_t)ax, xb = (int16_t)bx;
        if (xa > xb) { int16_t t = xa; xa = xb; xb = t; }
        sh1106_draw_hline(dev, xa, y, xb - xa + 1, color);
        ax += inv1 * step;
        bx += inv2 * step;
    }
}

void sh1106_fill_triangle(sh1106_t *dev,
                          int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          sh1106_color_t color)
{
    /* Sort vertices by y (bubble sort, 3 elements) */
    if (y0 > y1) { int16_t t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
    if (y0 > y2) { int16_t t; t=x0;x0=x2;x2=t; t=y0;y0=y2;y2=t; }
    if (y1 > y2) { int16_t t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }

    if (y1 == y2) {
        _tri_fill_flat(dev, x0, y0, x1, y1, x2, y2, color);
    } else if (y0 == y1) {
        _tri_fill_flat(dev, x2, y2, x0, y0, x1, y1, color);
    } else {
        /* Split into flat-bottom + flat-top halves */
        int16_t xm = (int16_t)(x0 + (float)(y1 - y0) / (y2 - y0) * (x2 - x0));
        _tri_fill_flat(dev, x0, y0, x1, y1, xm,  y1, color);
        _tri_fill_flat(dev, x2, y2, x1, y1, xm,  y1, color);
    }
}

/* --- Bitmap --- */

void sh1106_draw_bitmap(sh1106_t *dev, int16_t x, int16_t y,
                        const uint8_t *bitmap, int16_t w, int16_t h,
                        sh1106_color_t color)
{
    int16_t bytes_per_row = (w + 7) / 8;
    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            uint8_t byte = bitmap[row * bytes_per_row + col / 8];
            if (byte & (0x80u >> (col % 8)))
                sh1106_draw_pixel(dev, x + col, y + row, color);
        }
    }
}

/* =========================================================================
 * Text rendering
 * ========================================================================= */

int16_t sh1106_draw_char(sh1106_t *dev, int16_t x, int16_t y,
                         char c, const sh1106_font_t *font,
                         sh1106_color_t color)
{
    if (!font || c < (char)font->first_char || c > (char)font->last_char)
        return 0;

    uint8_t  pages      = font->height / 8;
    uint16_t glyph_idx  = (uint8_t)(c - font->first_char);
    uint16_t glyph_size = font->width * pages;
    const uint8_t *glyph = &font->data[glyph_idx * glyph_size];

    for (uint8_t col = 0; col < font->width; col++) {
        for (uint8_t pg = 0; pg < pages; pg++) {
            uint8_t byte = glyph[col * pages + pg];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (byte & (1u << bit))
                    sh1106_draw_pixel(dev, x + col, y + pg * 8 + bit, color);
            }
        }
    }
    return font->width + font->spacing;
}

void sh1106_draw_string(sh1106_t *dev, int16_t x, int16_t y,
                        const char *str, const sh1106_font_t *font,
                        sh1106_color_t color)
{
    if (!str || !font) return;
    int16_t x_orig = x;
    while (*str) {
        if (*str == '\n') {
            x  = x_orig;
            y += font->height;
        } else {
            x += sh1106_draw_char(dev, x, y, *str, font, color);
        }
        str++;
    }
}

void sh1106_draw_new_string(sh1106_t *dev, int16_t x, int16_t y,
                            const char *str, const sh1106_font_t *font,
                            sh1106_color_t color)
{
    /* Erase the string area to ensure full coverage */
    int16_t width = sh1106_string_width(str, font);

    sh1106_fill_rect(dev, x, y, width, font->height, SH1106_BLACK);
    sh1106_draw_string(dev, x, y, str, font, color);
}

int16_t sh1106_string_width(const char *str, const sh1106_font_t *font)
{
    if (!str || !font) return 0;
    int16_t w = 0;
    while (*str) {
        if (*str != '\n' &&
            *str >= (char)font->first_char &&
            *str <= (char)font->last_char)
        {
            w += font->width + font->spacing;
        }
        str++;
    }
    return w;
}
