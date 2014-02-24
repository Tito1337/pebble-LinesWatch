#include <pebble.h>

#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}

/* CONSTANTS
    - BackgroundColor is the main filling (black)
    - ForegroundColor is the filling of the cross, points and segments (white)
    - AnimationTime is the duration in milliseconds of transitions (2000)
    - Points are the coordinates of the two points at of each quadrant */
const GColor BackgroundColor = GColorBlack;
const GColor ForegroundColor = GColorWhite;
const int AnimationTime = 2000;

const GRect Points[2] = {
    ConstantGRect(33, 25, 4, 4),
    ConstantGRect(33, 53, 4, 4)
};

/* Each number is contained in a quadrant which has a layer (its coordinates), 
    2 permanent points and 8 possible segments. Also storing the animations
    that need to be globally accessible and the current segments byte for
    future comparison. */
typedef struct {
    Layer *layer;
    Layer *points[2];
    Layer *segments[8];
    PropertyAnimation *animations[8];
    char currentSegments;
} Quadrant;
Quadrant quadrants[4];

/* Each quadrant has 8 possible segments, whose coordinates are expressed here.
    There are two coordinates for each segment : 
    - visible is the rectangle when the segment is displayed
    - invisible is the rectangle/line to which the segment retracts when
      disappearing. */
typedef struct {
    GRect visible;
    GRect invisible;
} Segment;
Segment Segments[8] = {
    {ConstantGRect(29, 0, 4, 29), ConstantGRect(29, 29, 4, 0)},
    {ConstantGRect(33, 53, 4, 29), ConstantGRect(33, 57, 4, 0)},
    {ConstantGRect(33, 53, 37, 4), ConstantGRect(37, 53, 0, 4)},
    {ConstantGRect(33, 25, 4, 32), ConstantGRect(33, 53, 4, 0)},
    {ConstantGRect(0, 53, 37, 4), ConstantGRect(33, 53, 0, 4)},
    {ConstantGRect(33, 25, 37, 4), ConstantGRect(37, 25, 0, 4)},
    {ConstantGRect(33, 0, 4, 29), ConstantGRect(33, 25, 4, 0)},
    {ConstantGRect(0, 25, 37, 4), ConstantGRect(33, 25, 0, 4)}
};

/* Other globals */
Window *window;
Layer *cross;

/*******************/
/* GENERAL PURPOSE */
/*******************/
/* fill_layer ensures that a layer is always filled with the ForegroundColor */
void fill_layer(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

/* draw_cross draws the main cross (horizontal and vertical lines accross
    the screen */
void draw_cross(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, GRect(70, 0, 4, 168), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0, 82, 144, 4), 0, GCornerNone);
}

static void destroy_property_animation(PropertyAnimation **prop_animation) {
  if (*prop_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *prop_animation)) {
    animation_unschedule((Animation*) *prop_animation);
  }

  property_animation_destroy(*prop_animation);
  *prop_animation = NULL;
}

/************/
/* SEGMENTS */
/************/
/* segment_show draws a segment with an animation */
void segment_show(Quadrant *quadrant, int id) {
	GRect visible = Segments[id].visible;
    GRect invisible = Segments[id].invisible;

    /* Ensures the segment is not animating to prevent bugs */
    destroy_property_animation(&quadrant->animations[id]);
    
    quadrant->animations[id] = property_animation_create_layer_frame(quadrant->segments[id], &invisible, &visible);
    animation_set_duration(&quadrant->animations[id]->animation, AnimationTime);
    animation_set_curve(&quadrant->animations[id]->animation, AnimationCurveLinear);
    animation_schedule(&quadrant->animations[id]->animation);
}

/* segment_hide removes a segment with an animation */
void segment_hide(Quadrant *quadrant, int id) {
	GRect visible = Segments[id].visible;
    GRect invisible = Segments[id].invisible;

    /* Ensures the segment is not animating to prevent bugs */
    destroy_property_animation(&quadrant->animations[id]);
    
    quadrant->animations[id] = property_animation_create_layer_frame(quadrant->segments[id], &visible, &invisible);
    animation_set_duration(&quadrant->animations[id]->animation, AnimationTime);
    animation_set_curve(&quadrant->animations[id]->animation, AnimationCurveLinear);
    animation_schedule(&quadrant->animations[id]->animation);
}

/* segment_draw calculates which segments need to be showed or hided.
    new is a byte, each bit of which represent one of the 8 segments */
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
/* quadrant_init initializes the layer of a quadrant and the layers of
    its points and segments */
void quadrant_init(Quadrant *quadrant, GRect coordinates) {
    quadrant->layer = layer_create(coordinates);

    /* Two points visible for 6, 8 and 9 (always present) */
    for(int i=0; i<2; i++) {
        quadrant->points[i] = layer_create(Points[i]);
        layer_set_update_proc(quadrant->points[i], fill_layer);
        layer_add_child(quadrant->layer, quadrant->points[i]);
    }    
    
    /* Now we create the 8 segments, invisible */
    for(int i=0; i<8; i++) {
        quadrant->segments[i] = layer_create(Segments[i].invisible);
        layer_set_update_proc(quadrant->segments[i], fill_layer);
        layer_add_child(quadrant->layer, (quadrant->segments[i]));
    }

    quadrant->currentSegments = 0b00000000;
}

/* quadrant_deinit runs through the same as quadrant_init, but calls *_destroy
    where the former called *_create */
void quadrant_deinit(Quadrant *quadrant) {
    layer_destroy(quadrant->layer);

    for(int i=0; i<2; i++) {
        layer_destroy(quadrant->points[i]);
    }

    for(int i=0; i<8; i++) {
        layer_destroy(quadrant->segments[i]);
    	destroy_property_animation(&quadrant->animations[i]);
    }

    quadrant->currentSegments = 0b00000000;
}

/* quadrant_number calculates the segments to show for the inputted number. The
    8 segments are expressed in a byte, each bit of which represent a segment */
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

/****************/
/* MAIN METHODS */
/****************/

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

/* init is called when the watchface is loaded on screen. It initializes
    various layers for the main design */
void init() {
    /* Window inits */
    window = window_create();
    window_stack_push(window, true /* Animated */);
    window_set_background_color(window, BackgroundColor);

    Layer *window_layer = window_get_root_layer(window);
    
    /* Cross */
    cross = layer_create(GRect(0, 0, 144, 168));
    layer_set_update_proc(cross, draw_cross);
    layer_add_child(window_layer, cross);
    
    /* Quadrants */
    /* Each quarter of screen is 70x82 pixels */
    quadrant_init(&quadrants[0], GRect(0, 0, 70, 82));
    quadrant_init(&quadrants[1], GRect(74, 0, 70, 82));
    quadrant_init(&quadrants[2], GRect(0, 86, 70, 82));
    quadrant_init(&quadrants[3], GRect(74, 86, 70, 82));
    
    layer_add_child(window_layer, quadrants[0].layer);
    layer_add_child(window_layer, quadrants[1].layer);
    layer_add_child(window_layer, quadrants[2].layer);
    layer_add_child(window_layer, quadrants[3].layer);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

/* deinit runs through the same as init, but calls *_destroy
    where the former called *_create */
void deinit() {
    tick_timer_service_unsubscribe();

    quadrant_deinit(&quadrants[0]);
    quadrant_deinit(&quadrants[1]);
    quadrant_deinit(&quadrants[2]);
    quadrant_deinit(&quadrants[3]);

    layer_destroy(cross);
    window_destroy(window);
}

/* handle_minute_tick is called at every minute/time change. It gets the hour,
    minute and sends the numbers to each quadrant */
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

    int hour, min;
    min = tick_time->tm_min;
    hour = tick_time->tm_hour;

    if(!clock_is_24h_style()) {
        hour = hour%12;
        if(hour == 0) hour=12;
    }
    
    quadrant_number(&quadrants[0], hour/10);
    quadrant_number(&quadrants[1], hour%10);
    quadrant_number(&quadrants[2], min/10);
    quadrant_number(&quadrants[3], min%10);
}

/* main pebble sets */
int main(void) {

    init();
    app_event_loop();
    deinit();

}
