#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x1F, 0x8A, 0x52, 0xAB, 0x4E, 0x33, 0x42, 0x26, 0xA8, 0x9F, 0x46, 0xAB, 0x2A, 0xB5, 0x00, 0xB5 }
PBL_APP_INFO(MY_UUID,
             "LinesWatch", "Tito",
             0, 1, /* App version */
             INVALID_RESOURCE,
             APP_INFO_WATCH_FACE);


Window window;
Layer crossLayer, quadrant0, quadrant1, quadrant2, quadrant3;

void draw_cross(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(70, 0, 4, 168), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0, 82, 144, 4), 0, GCornerNone);
}

/* draw_bars will put the bars in a layer.
    There are 7 bars for each possible number : _|_
                                                _|_
                                                 |
    Those are numbered from 0 to 6 and we represent the whole by a byte (char).
    For examble 0b10000000 will draw the first bar. A more complex example :
    0b01010010 will draw  | which is the number 4.
                         _
                          |                                                   
    NB: The last bit is useless                                               */
void draw_bars(char byte, Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    /* We draw the two points visible for 6, 8 and 9 */
    graphics_fill_rect(ctx, GRect(33, 25, 4, 4), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(33, 53, 4, 4), 0, GCornerNone);
    
    if(((byte & ( 1 << 7 )) >> 7) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(0, 25, 37, 4), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 6 )) >> 6) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(33, 0, 4, 29), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 5 )) >> 5) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(33, 25, 37, 4), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 4 )) >> 4) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(0, 53, 37, 4), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 3 )) >> 3) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(33, 25, 4, 32), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 2 )) >> 2) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(33, 53, 37, 4), 0, GCornerNone);    
    }
    if(((byte & ( 1 << 1 )) >> 1) == 0b00000001) {
        graphics_fill_rect(ctx, GRect(33, 53, 4, 29), 0, GCornerNone);    
    }
}

/* draw_number will convert a human readable number to draw_bars */
static void draw_number(char number, Layer *layer, GContext *ctx) {
    switch(number) {
        case 0:
            draw_bars(0b00001000, layer, ctx);
            break;
        case 1:
            draw_bars(0b01001010, layer, ctx);
            break;
        case 2:
            draw_bars(0b10000100, layer, ctx);
            break;
        case 3:
            draw_bars(0b10010000, layer, ctx);
            break;
        case 4:
            draw_bars(0b01010010, layer, ctx);
            break;
        case 5:
            draw_bars(0b00110000, layer, ctx);
            break;
        case 6:
            draw_bars(0b00100000, layer, ctx);
            break;
        case 7:
            draw_bars(0b10001010, layer, ctx);
            break;
        case 8:
            draw_bars(0b00000000, layer, ctx);
            break;
        case 9:
            draw_bars(0b00010000, layer, ctx);
            break;
    }
}

int hour, min;
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *tickE) {
    PblTm t;
    get_time(&t);
    min = t.tm_min;
    hour = t.tm_hour;

    if(!clock_is_24h_style()) {
        hour = hour%12;
        if(hour == 0) hour=12;
    }

    layer_mark_dirty(&quadrant0);
    layer_mark_dirty(&quadrant1);
    layer_mark_dirty(&quadrant2);
    layer_mark_dirty(&quadrant3);
}

void draw_quadrant0(Layer *layer, GContext *ctx) {
    draw_number(hour/10, layer, ctx);
}

void draw_quadrant1(Layer *layer, GContext *ctx) {
    draw_number(hour%10, layer, ctx);
}

void draw_quadrant2(Layer *layer, GContext *ctx) {
    draw_number(min/10, layer, ctx);
}

void draw_quadrant3(Layer *layer, GContext *ctx) {
    draw_number(min%10, layer, ctx);
}

void handle_init(AppContextRef ctx) {
    (void)ctx;

    /* Window inits */
    window_init(&window, "LinesWatch");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    /* Cross */
    layer_init(&crossLayer, GRect(0, 0, 144, 168));
    crossLayer.update_proc = draw_cross;

    /* Each quarter of screen is 70x82 pixels */
    layer_init(&quadrant0, GRect(0, 0, 70, 82));
    quadrant0.update_proc = draw_quadrant0;

    layer_init(&quadrant1, GRect(74, 0, 70, 82));
    quadrant1.update_proc = draw_quadrant1;
    
    layer_init(&quadrant2, GRect(0, 86, 70, 82));
    quadrant2.update_proc = draw_quadrant2;
    
    layer_init(&quadrant3, GRect(74, 86, 70, 82));
    quadrant3.update_proc = draw_quadrant3;
    
    /* Draw */
    handle_minute_tick(ctx, NULL);
    layer_add_child(&window.layer, &crossLayer);
    layer_add_child(&window.layer, &quadrant0);
    layer_add_child(&window.layer, &quadrant1);
    layer_add_child(&window.layer, &quadrant2);
    layer_add_child(&window.layer, &quadrant3);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }

  };
  app_event_loop(params, &handlers);
}
