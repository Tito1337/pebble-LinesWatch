#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x1F, 0x8A, 0x52, 0xAB, 0x4E, 0x33, 0x42, 0x26, 0xA8, 0x9F, 0x46, 0xAB, 0x2A, 0xB5, 0x00, 0xB5 }
PBL_APP_INFO(MY_UUID,
             "LinesWatch", "Tito",
             2, 0, /* App version */
             INVALID_RESOURCE,
             APP_INFO_WATCH_FACE);

/* TYPE DEFS */
#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}
typedef struct {
    GRect visible;
    GRect invisible;
} Segment;
typedef struct {
    Layer layer;
    Layer points[2];
    Layer segments[8];
    PropertyAnimation animations[8];
    char currentSegments;
} Quadrant;

/* CONSTANTS */
const GColor BackgroundColor = GColorBlack;
const GColor ForegroundColor = GColorWhite;
const int AnimationTime = 2000;

const GRect Points[2] = {
    ConstantGRect(33, 25, 4, 4),
    ConstantGRect(33, 53, 4, 4)
};

const Segment Segments[8] = {
    {ConstantGRect(29, 0, 4, 29), ConstantGRect(29, 29, 4, 0)},
    {ConstantGRect(33, 53, 4, 29), ConstantGRect(33, 57, 4, 0)},
    {ConstantGRect(33, 53, 37, 4), ConstantGRect(37, 53, 0, 4)},
    {ConstantGRect(33, 25, 4, 32), ConstantGRect(33, 53, 4, 0)},
    {ConstantGRect(0, 53, 37, 4), ConstantGRect(33, 53, 0, 4)},
    {ConstantGRect(33, 25, 37, 4), ConstantGRect(37, 25, 0, 4)},
    {ConstantGRect(33, 0, 4, 29), ConstantGRect(33, 25, 4, 0)},
    {ConstantGRect(0, 25, 37, 4), ConstantGRect(33, 25, 0, 4)}
};

/* Globals */
Window window;
int hour, min;
Layer cross;
Quadrant quadrants[4];

void draw_cross(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, GRect(70, 0, 4, 168), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0, 82, 144, 4), 0, GCornerNone);
}

void fill_layer(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, layer->bounds, 0, GCornerNone);
}

/************/
/* SEGMENTS */
/************/
void segment_show(Quadrant *quadrant, int id) {
    GRect visible = Segments[id].visible;
    GRect invisible = Segments[id].invisible;
    property_animation_init_layer_frame(&quadrant->animations[id], &quadrant->segments[id], &invisible, &visible);
    animation_set_duration(&quadrant->animations[id].animation, AnimationTime);
    animation_set_curve(&quadrant->animations[id].animation, AnimationCurveLinear);
    animation_schedule(&quadrant->animations[id].animation);
}
void segment_hide(Quadrant *quadrant, int id) {
    GRect visible = Segments[id].visible;
    GRect invisible = Segments[id].invisible;
    property_animation_init_layer_frame(&quadrant->animations[id], &quadrant->segments[id], &visible, &invisible);
    animation_set_duration(&quadrant->animations[id].animation, AnimationTime);
    animation_set_curve(&quadrant->animations[id].animation, AnimationCurveLinear);
    animation_schedule(&quadrant->animations[id].animation);
}
void segment_draw(Quadrant *quadrant, char new) {
    char prev = quadrant->currentSegments;
    char compare = prev^new;
    for(int i=0; i<8; i++) {
        if(((compare & ( 1 << i )) >> i) == 0b00000001) {
            if(((new & ( 1 << i )) >> i) == 0b00000001) {
                segment_show(quadrant, i);
            } else {
                segment_hide(quadrant, i);
            }
        }
    }
    
    quadrant->currentSegments = new;
}

/*************/
/* QUADRANTS */
/*************/
void quadrant_init(Quadrant *quadrant, GRect coordinates, GContext *ctx) {
    layer_init(&quadrant->layer, coordinates);

    /* Two points visible for 6, 8 and 9 (always present) */
    for(int i=0; i<2; i++) {
        layer_init(&(quadrant->points[i]), Points[i]);
        quadrant->points[i].update_proc = fill_layer;
        layer_add_child(&quadrant->layer, &(quadrant->points[i]));        
    }    
    
    /* Now we create the 8 segments, invisible */
    for(int i=0; i<8; i++) {
        layer_init(&(quadrant->segments[i]), Segments[i].invisible);
        quadrant->segments[i].update_proc = fill_layer;
        layer_add_child(&quadrant->layer, &(quadrant->segments[i]));
    }

    quadrant->currentSegments = 0b00000000;
}

void quadrant_number(Quadrant *quadrant, int number) {
    switch(number) {
        case 0:
            segment_draw(quadrant, 0b00001000);
            break;
        case 1:
            segment_draw(quadrant, 0b00001011);
            break;
        case 2:
            segment_draw(quadrant, 0b10000100);
            break;
        case 3:
            segment_draw(quadrant, 0b10010000);
            break;
        case 4:
            segment_draw(quadrant, 0b01010010);
            break;
        case 5:
            segment_draw(quadrant, 0b00110000);
            break;
        case 6:
            segment_draw(quadrant, 0b00100000);
            break;
        case 7:
            segment_draw(quadrant, 0b10001010);
            break;
        case 8:
            segment_draw(quadrant, 0b00000000);
            break;
        case 9:
            segment_draw(quadrant, 0b00010000);
            break;
    }
}

void handle_init(AppContextRef ctx) {
    /* Window inits */
    window_init(&window, "LinesWatch");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, BackgroundColor);
    
    /* Cross */
    layer_init(&cross, GRect(0, 0, 144, 168));
    cross.update_proc = draw_cross;
    layer_add_child(&window.layer, &cross);
    
    /* Quadrants */
    /* Each quarter of screen is 70x82 pixels */
    quadrant_init(&quadrants[0], GRect(0, 0, 70, 82), ctx);
    quadrant_init(&quadrants[1], GRect(74, 0, 70, 82), ctx);
    quadrant_init(&quadrants[2], GRect(0, 86, 70, 82), ctx);
    quadrant_init(&quadrants[3], GRect(74, 86, 70, 82), ctx);
    
    layer_add_child(&window.layer, &quadrants[0].layer);
    layer_add_child(&window.layer, &quadrants[1].layer);
    layer_add_child(&window.layer, &quadrants[2].layer);
    layer_add_child(&window.layer, &quadrants[3].layer);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *tickE) {
    PblTm t;
    get_time(&t);
    min = t.tm_min;
    hour = t.tm_hour;

    if(!clock_is_24h_style()) {
        hour = hour%12;
        if(hour == 0) hour=12;
    }
    
    quadrant_number(&quadrants[0], hour/10);
    quadrant_number(&quadrants[1], hour%10);
    quadrant_number(&quadrants[2], min/10);
    quadrant_number(&quadrants[3], min%10);
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
