#include <pebble.h>
#include <clock.h>
#include <resources.h>
#include <constants.h>
#include <palette.h>
#include <persistence.h>

static Layer* s_clockLayer;
static char s_timeBuffer[CLOCK_TEXT_SIZE];
static char s_dateBuffer[CLOCK_TEXT_SIZE];

static BatteryChargeState s_battery;
static bool s_bluetoothStatus;

static GPoint s_spoogelet[N_SPOOGELET];
static int16_t s_spoogeletVx[N_SPOOGELET];
static int16_t s_spoogeletVy[N_SPOOGELET];
static GPoint s_attractor;

static uint8_t s_clockTickCount = 0;

static uint8_t s_clockPixelOffset = 2;
static bool s_flashMainFace = false;

#define CLOCK_OFFSET 16

void setClockPixelOffset(uint8_t offset) {
  s_clockPixelOffset = offset;
}

void updateClockLayer() {
  if (s_clockLayer) layer_mark_dirty(s_clockLayer);
}

void clockAnimReset(TimeUnits units_changed) {
  s_clockTickCount = 0;
  for (unsigned i = 0; i < N_SPOOGELET; ++i) {
    s_spoogelet[i].x = (7*WIN_SIZE_X/16 + (rand() % WIN_SIZE_X/8)) * SUB_PIXEL;
    s_spoogelet[i].y = (30 + (rand() % 10)) * SUB_PIXEL;

    int32_t angle = rand() % TRIG_MAX_ANGLE;
    int32_t v = ANIM_MIN_V + (rand() % (ANIM_MAX_V-ANIM_MIN_V));
    s_spoogeletVx[i] =  sin_lookup(angle) * v / TRIG_MAX_RATIO;
    s_spoogeletVy[i] = -cos_lookup(angle) * v / TRIG_MAX_RATIO;
  }
  s_attractor.x = 112;//130
  s_attractor.y = 85;//60
}

bool clockAnimCallback(TimeUnits units_changed) {

  int16_t strength = 0;
  if (s_clockTickCount > 24) strength = s_clockTickCount;

  // Hour+ specific animation
  if ((units_changed & HOUR_UNIT) > 0 && s_clockTickCount % 4 == 0) {
    s_flashMainFace = !s_flashMainFace;
  }

  // Month+ specific animation
  if ((units_changed & MONTH_UNIT) > 0 && s_clockTickCount % 6 == 0) {
    colourOverride( rand() % PALETTE_MAX );
  }

  if ((units_changed & YEAR_UNIT) > 0 && s_clockTickCount % 8 == 0) {
    uint8_t bgColourOverride = rand() % PALETTE_MAX;
    while ( bgColourOverride == getColour() ) bgColourOverride = rand() % PALETTE_MAX;
    window_set_background_color( layer_get_window(s_clockLayer), getBGFlashColour(bgColourOverride) );
  }

  for (unsigned i = 0; i < N_SPOOGELET; ++i) {

    s_spoogelet[i].x += s_spoogeletVx[i];
    s_spoogelet[i].y += s_spoogeletVy[i];

    int16_t xOff = ((s_attractor.x * SUB_PIXEL) - s_spoogelet[i].x);
    int16_t yOff = ((s_attractor.y * SUB_PIXEL) - s_spoogelet[i].y);

    // I go from 0 to 50, at 50 i need to set the offset so that they will be on top off each other
    // before that, it should nudge closer

    xOff = (xOff * strength) / 100;
    yOff = (yOff * strength) / 100; // nominally FPS*2 = 50

    s_spoogelet[i].x += xOff;
    s_spoogelet[i].y += yOff;
  }

  updateClockLayer(); // Redraw

  if (++s_clockTickCount == ANIM_FRAMES) {
    s_flashMainFace = false;
    colourOverride( -1 );
    window_set_background_color( layer_get_window(s_clockLayer), GColorBlack );
    return false;
  } else {
    return true; // Request more frames
  }
}

void draw3DText(GContext *ctx, GRect loc, GFont* f, char* buffer, uint8_t offset, bool BWMode, GColor BWFg, GColor BWBg) {

  if (BWMode) graphics_context_set_text_color(ctx, BWBg);

  // corners
  if (!BWMode) graphics_context_set_text_color(ctx, getTextColourL());
  loc.origin.x -= offset; // CL
  loc.origin.y += offset; // UL
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  if (!BWMode) graphics_context_set_text_color(ctx, getTextColourC());
  loc.origin.x += offset; // CU
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  loc.origin.x += offset; // RU
  if (!BWMode) graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);


  if (!BWMode) graphics_context_set_text_color(ctx, getTextColourR());
  loc.origin.y -= offset; // CR
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  loc.origin.y -= offset; // DR
  if (!BWMode) graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);


  if (!BWMode) graphics_context_set_text_color(ctx, getTextColourD());
  loc.origin.x -= offset; // DC
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  loc.origin.x -= offset; // DR
  if (!BWMode) graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  if (!BWMode) graphics_context_set_text_color(ctx, getTextColourL());
  loc.origin.y += offset; // CR
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // main
  if (BWMode) graphics_context_set_text_color(ctx, BWFg);
  else if (s_flashMainFace) graphics_context_set_text_color(ctx, getTextColourU());
  else graphics_context_set_text_color(ctx, getTextColourC());
  loc.origin.x += offset; // O
  graphics_draw_text(ctx, buffer, *f, loc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

void updateTimeBuffer() {
  // Do the clock
  time_t temp = time(NULL);
  struct tm *tickTime = localtime(&temp);
  // Write the current hours and minutes and maybe secods into the buffer
  if (getUserOpt(OPT_SHOW_SECONDS) == true) {
    if(clock_is_24h_style() == true) {
      strftime(s_timeBuffer, CLOCK_TEXT_SIZE*sizeof(char), "%H:%M:%S", tickTime);
    } else {
      strftime(s_timeBuffer, CLOCK_TEXT_SIZE*sizeof(char), "%I:%M:%S", tickTime);
    }
  } else {
    if(clock_is_24h_style() == true) {
      strftime(s_timeBuffer, CLOCK_TEXT_SIZE*sizeof(char), "%H:%M", tickTime);
    } else {
      strftime(s_timeBuffer, CLOCK_TEXT_SIZE*sizeof(char), "%I:%M", tickTime);
    }
  }
}

void updateDateBuffer() {
  time_t temp = time(NULL);
  struct tm *tickTime = localtime(&temp);
  strftime(s_dateBuffer, CLOCK_TEXT_SIZE*sizeof(char), "%e %b", tickTime); // 22 May
  updateClockLayer();
}

void updateBattery(BatteryChargeState charge) {
  s_battery = charge;
  updateClockLayer();
}

void updateBluetooth(bool bluetooth) {
  s_bluetoothStatus = bluetooth;
  updateClockLayer();
}


static void clock_update_proc(Layer *this_layer, GContext *ctx) {
  GRect b = layer_get_bounds(this_layer);
  const int clockUpgrade = getUserSetting(SETTING_TECH);

  // TODO charge status
  // BATTERY + BT
  if (clockUpgrade >= TECH_BATTERY) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, getLiquidTimeHighlightColour());
    graphics_draw_rect(ctx, GRect(119,9,18,7));
    graphics_draw_rect(ctx, GRect(137,11,2,3));
    graphics_fill_rect(ctx, GRect(121,11,s_battery.charge_percent/7,3), 0, GCornersAll); // 100%=14 pixels
    if (s_bluetoothStatus) {
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_context_set_fill_color(ctx, getLiquidTimeColour());
      graphics_fill_rect(ctx, GRect(8, 6, 5, 12), 0, GCornersAll);
      drawBitmap(ctx, getBluetoothImage(), GRect(7, 5, 7, 14));
    }
  }

  // DATE
  if (clockUpgrade >= TECH_MONTH) {
    GRect dateRect = GRect(b.origin.x, b.origin.y, b.size.w, 30);
    draw3DText(ctx, dateRect, getClockSmallFont(), s_dateBuffer, 1, false, GColorBlack, GColorBlack);
  }

  int makeRoomForTheDateOffset = 0;
  if (clockUpgrade == 0) makeRoomForTheDateOffset = -10;
  GRect timeRect = GRect(b.origin.x, b.origin.y + CLOCK_OFFSET + makeRoomForTheDateOffset, b.size.w, b.size.h - CLOCK_OFFSET - makeRoomForTheDateOffset);

  draw3DText(ctx, timeRect, getClockFont(), s_timeBuffer, s_clockPixelOffset, false, GColorBlack, GColorBlack);

  if (s_clockTickCount == 0) return; // No animation in progress

  graphics_context_set_stroke_color(ctx, GColorBlack);
  for (unsigned i = 0; i < N_SPOOGELET; ++i) {
    const GPoint p = GPoint(s_spoogelet[i].x / SUB_PIXEL, s_spoogelet[i].y / SUB_PIXEL);
    if ((p.x - s_attractor.x) < 8 && (p.x - s_attractor.x) > -8 && (p.y - s_attractor.y) < 8 && (p.y - s_attractor.y) > -8) continue; // Too close
    const uint8_t r = 1+rand()%3;
    if (i%2==0) graphics_context_set_fill_color(ctx, getSpoogicalColourA());
    else        graphics_context_set_fill_color(ctx, getSpoogicalColourB());
    graphics_fill_circle(ctx, p, r);
    graphics_draw_circle(ctx, p, r);
  }

}

void create_clock_layer(Window* parentWindow) {
  // Create the clock layer in the top 1/3 of the screen
  Layer* window_layer = window_get_root_layer(parentWindow);
  GRect window_bounds = layer_get_bounds(window_layer);
  s_clockLayer = layer_create( GRect(0, 0, window_bounds.size.w, window_bounds.size.h/3) );
  // Add as child of the main window layer and set callback
  layer_add_child(window_layer, s_clockLayer);
  layer_set_update_proc(s_clockLayer, clock_update_proc);
  layer_set_clips(s_clockLayer, false);
  battery_state_service_subscribe(updateBattery);
  s_battery = battery_state_service_peek();
  bluetooth_connection_service_subscribe(updateBluetooth);
  s_bluetoothStatus = bluetooth_connection_service_peek();
  updateTimeBuffer();
  updateDateBuffer();
}


void destroy_clock_layer() {
  layer_destroy(s_clockLayer);
  s_clockLayer = 0;
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
}
