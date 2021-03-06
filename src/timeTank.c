#include <pebble.h>
#include "timeTank.h"
#include "timeStore.h"
#include "constants.h"
#include "resources.h"
#include "clock.h"
#include "palette.h"
#include "persistence.h"

// Frame to hold tank gfx
static Layer* s_tankLayer;

static uint8_t s_tankTickCount;
static uint8_t s_waterfallOffset;
static uint64_t s_tankAnimToAdd;
static uint64_t s_tankAnimRemainder;

// Location of bits in the liquid
static GPoint liquid_bits_bgn[N_LIQUID_BITS];
static GPoint liquid_bits_end[N_LIQUID_BITS];
static uint16_t liquid_bit_y[N_LIQUID_BITS];

static GPoint waterfall_bits_bgn[N_WATERFALL_BITS];
static GPoint waterfall_bits_end[N_WATERFALL_BITS];


void randomiseWaterfallBits() {
  for (unsigned i = 0; i < N_WATERFALL_BITS; ++i) {
    int16_t x = 102 + rand()%8;
    int16_t y = -4 - rand()%48;
    waterfall_bits_bgn[i].x = x;
    waterfall_bits_end[i].x = x;
    waterfall_bits_bgn[i].y = y;
    waterfall_bits_end[i].y = y + (2 + rand()%3);
  }
}

void update_timeTank_layer() {
  layer_mark_dirty(s_tankLayer);
}

void tankAnimReset(TimeUnits units_changed) {
  if (getDisplayTime() > getUserTime()) APP_LOG(APP_LOG_LEVEL_DEBUG, "DISP > USER ?!?!");
  s_tankAnimToAdd     = getUserTime() - getDisplayTime();
  s_tankAnimRemainder = s_tankAnimToAdd % (ANIM_FRAMES/2);
  s_tankAnimToAdd     /= (ANIM_FRAMES/2);
  s_waterfallOffset = 0;
  s_tankTickCount = 0;
}

bool tankAnimCallback(TimeUnits units_changed) {

  if (s_tankTickCount >= (ANIM_FRAMES/2)) {
    updateDisplayTime( getDisplayTime() + s_tankAnimToAdd );
    // Add the remainder on the first fram to make the value jump, then tend to the correct value
    if (s_tankTickCount == (ANIM_FRAMES/2)) updateDisplayTime( getDisplayTime() + s_tankAnimRemainder );
    update_timeTank_layer();
    s_waterfallOffset += WATERFALL_SPEED;
    for (unsigned i = 0; i < N_WATERFALL_BITS; ++i) {
      waterfall_bits_bgn[i].y += WATERFALL_SPEED;
      waterfall_bits_end[i].y += WATERFALL_SPEED;
    }
  }

  if (++s_tankTickCount == ANIM_FRAMES) {
    // Make sure we show the correct time at the end of the animation (there will be a modulus rounding error)
    updateDisplayTime( getUserTime() );
    randomiseWaterfallBits();
    return false;
  } else {
    return true; // Request more frames
  }
}

/**
 * Draw the TimeTank which occupies the bottom 1/3 of the screen and holds how much liquid time the user has
 * along with how full the tank is
 */
static void timeTank_update_proc(Layer *this_layer, GContext *ctx) {
  GRect tank_bounds = layer_get_bounds(this_layer);

  static char s_tankFullPercetText[TEXT_BUFFER_SIZE];
  static char s_tankContentText[TEXT_BUFFER_SIZE];

  unsigned _percentage;
  percentageToString(getDisplayTime(), getTankCapacity(), s_tankFullPercetText, TEXT_BUFFER_SIZE, &_percentage, true);
  timeToString(getDisplayTime(), s_tankContentText, TEXT_BUFFER_SIZE, true);

  // Fill back
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, tank_bounds, 10, GCornersAll);
  //draw_gradient_rect(ctx, tank_bounds,GColorDarkGray, GColorLightGray, TOP_TO_BOTTOM);

  // Fill screws
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  GPoint screw_one = GPoint(tank_bounds.origin.x + 1*tank_bounds.size.w/6 - 2, tank_bounds.origin.y + tank_bounds.size.h/3);
  GPoint screw_two = GPoint(tank_bounds.origin.x + 5*tank_bounds.size.w/6 - 2, tank_bounds.origin.y + tank_bounds.size.h/3);
  graphics_fill_circle(ctx, screw_one, 4);
  graphics_fill_circle(ctx, screw_two, 4);
  graphics_draw_circle(ctx, screw_one, 4);
  graphics_draw_circle(ctx, screw_two, 4);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(screw_one.x-2, screw_one.y-2), GPoint(screw_one.x+2, screw_one.y+2));
  graphics_draw_line(ctx, GPoint(screw_two.x+3, screw_two.y-1), GPoint(screw_two.x-3, screw_two.y+1));
  graphics_context_set_stroke_width(ctx, 1);

  graphics_context_set_fill_color(ctx, getLiquidTimeColour());
  graphics_context_set_stroke_color(ctx, getLiquidTimeHighlightColour());
  // If animation true - fill the pouring liquid
  if (s_tankTickCount >= ANIM_FRAMES/2) {
    const GRect waterfall = GRect(100, tank_bounds.origin.y - tank_bounds.size.h + s_waterfallOffset, 10, tank_bounds.size.h);
    graphics_fill_rect(ctx, waterfall, 3, GCornersAll);
    for (unsigned i = 0; i < N_WATERFALL_BITS; ++i) {
      graphics_draw_line(ctx, waterfall_bits_bgn[i], waterfall_bits_end[i]);
    }
  }
  // Fill liquid height
  int liquid_height = (tank_bounds.size.h * _percentage) / 100;
  GRect liquid_rect = GRect(0, tank_bounds.origin.y + tank_bounds.size.h - liquid_height, tank_bounds.size.w, liquid_height);
  graphics_fill_rect(ctx, liquid_rect, 0, GCornersAll);
  // Fill bits in the liquid
  for (unsigned i = 0; i < N_LIQUID_BITS; ++i) {
    liquid_bits_bgn[i].y = tank_bounds.origin.y + tank_bounds.size.h - liquid_height + liquid_bit_y[i];
    liquid_bits_end[i].y = liquid_bits_bgn[i].y;
    graphics_draw_line(ctx, liquid_bits_bgn[i], liquid_bits_end[i]);
  }

  // Fill borders
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_rect(ctx, GRect(tank_bounds.origin.x, tank_bounds.origin.y, tank_bounds.size.w,  tank_bounds.size.h));
  // Fill Frame
  graphics_context_set_stroke_width(ctx, 3);
  GRect middle_wall = GRect(tank_bounds.origin.x + 1, tank_bounds.origin.y + 0, tank_bounds.size.w - 2, tank_bounds.size.h - 0);
  GRect inner_wall  = GRect(tank_bounds.origin.x + 2, tank_bounds.origin.y + 2, tank_bounds.size.w - 4, tank_bounds.size.h - 4);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_round_rect(ctx, middle_wall, 10);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, inner_wall, 10);

  // Fill the text

  GRect text_percentage_rect = GRect(tank_bounds.origin.x + 5,
                                     tank_bounds.origin.y + TANK_TEXT_V_OFFSET,
                                     tank_bounds.size.w-10,
                                     TANK_TEXT_HEIGHT);

  GRect text_content_rect = GRect(tank_bounds.origin.x + 5,
                                  tank_bounds.origin.y + TANK_TEXT_V_OFFSET + TANK_TEXT_HEIGHT,
                                  tank_bounds.size.w - 10,
                                  TANK_TEXT_HEIGHT);


  // graphics_context_set_text_color(ctx, GColorWhite);

  draw3DText(ctx, text_percentage_rect, getGothic24BoldFont(), s_tankFullPercetText, 1, true, GColorWhite, GColorBlack);
  draw3DText(ctx, text_content_rect,    getGothic24BoldFont(), s_tankContentText,    1, true, GColorWhite, GColorBlack);
  //graphics_draw_text(ctx, s_tankContentText, *getGothic24BoldFont(), text_content_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  //graphics_draw_text(ctx, s_tankFullPercetText, *getGothic24BoldFont(), text_percentage_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Drew text");
}

void create_timeTank_layer(Window* parentWindow) {
  // Create the timeTank layer in the bottom 1/3 of the screen
  Layer* window_layer = window_get_root_layer(parentWindow);
  GRect window_bounds = layer_get_bounds(window_layer);
  s_tankLayer = layer_create( GRect(0, 2*(window_bounds.size.h/3), window_bounds.size.w, window_bounds.size.h/3) );
  // Add as child of the main window layer and set callback
  layer_add_child(window_layer, s_tankLayer);
  layer_set_update_proc(s_tankLayer, timeTank_update_proc);

  // Randomise liquid bits
  for (unsigned i = 0; i < N_LIQUID_BITS; ++i) {
    liquid_bits_bgn[i].x = 5 + rand()%130;
    liquid_bits_end[i].x =liquid_bits_bgn[i].x + (2 + rand()%3);
    liquid_bit_y[i] = 3 + rand()%50;
  }
  randomiseWaterfallBits();
}

void destroy_timeTank_layer() {
  layer_destroy(s_tankLayer);
}
