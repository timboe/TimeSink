#include <pebble.h>
#include <limits.h> // ULLONG_MAX
#include "timeSieve.h"
#include "constants.h"
#include "persistence.h"
#include "items.h"
#include "palette.h"
#include "resources.h"
#include "timeStore.h"

static Layer* s_timeSieveLayer;
static uint8_t s_sieveTickCount;

static GBitmap *s_convTopBitmap = NULL;
static GBitmap *s_convBotBitmap = NULL;
static GBitmap* s_convCap = NULL;
static GBitmap* s_sieveBasic = NULL;
static GBitmap* s_gem = NULL;

static uint8_t s_convOffset = 0;

static GRect s_treasureRect;
static bool s_treasureOnShow;
static bool s_multipleTreasures;
static int8_t s_treasureID;
static int8_t s_itemID;
static AppTimer* s_treasureTimeout = NULL;

static GPoint s_halo;
static uint8_t s_haloRings = 0;

static Layer* s_notifyLayer = NULL;
static int8_t s_notifyTreasureID = -1;
static int8_t s_notifyAchievementID = -1;
static int8_t s_notifyItemID;

static const GPathInfo FLAIR_PATH = {
  .num_points = 24,
  .points = (GPoint []) {{0, 0}, {100,  -10},  {100,  8},
                         {0, 0}, {-100, -11},  {-100, 7},
                         {0, 0}, {9,    100},  {-10,  100},
                         {0, 0}, {12,   -100}, {-9,   -100},
                         {0, 0}, {95,   100},  {100,  95},
                         {0, 0}, {-95,  -100}, {-100, -95},
                         {0, 0}, {95,   -100}, {100,  -95},
                         {0, 0}, {-95,  100},  {-100, 95}
                        }
};
static GPath* s_flairPath = NULL;
static int32_t s_flairAngle = 0;

void sieveAnimReset(TimeUnits units_changed) {
  s_sieveTickCount = 0;
  s_treasureRect = GRect(94, 18, 20, 20);
  s_halo = GPoint(104,30);
  s_haloRings = 0;
  s_flairAngle = 0;
}

bool sieveAnimCallback(TimeUnits units_changed) {
  // No animation if we are not moving a gem into place or doing BG flair
  if (s_treasureID == -1 && (units_changed & DAY_UNIT) == 0) return false;

  if (s_treasureID != -1) {
    if (++s_convOffset == 7) s_convOffset = 0; // Degenerency, 7 not 8 to avoid single-frame with no movement
    --s_treasureRect.origin.x;
    --s_halo.x;
    if (s_sieveTickCount % 14 == 0) ++s_haloRings;
  }

  // Day+ spec
  if ((units_changed & DAY_UNIT) > 0) {
    s_flairAngle += TRIG_MAX_ANGLE/ANIM_FPS/2;
  }

  if (++s_sieveTickCount == ANIM_FRAMES) {
    if (s_treasureID != -1) itemCanBeCollected();
    s_flairAngle = 0;
    return false;
  } else {
    return true; // Request more frames
  }
}

static void timeSieve_update_proc(Layer *this_layer, GContext *ctx) {
  GRect tank_bounds = layer_get_bounds(this_layer);

  // Backmost - FLAIR should only be drawn on DAY boundary
  if (s_flairAngle != 0) {
    graphics_context_set_fill_color(ctx, getLiquidTimeHighlightColour());
    graphics_context_set_stroke_color(ctx, getLiquidTimeColour());
    gpath_rotate_to(s_flairPath, s_flairAngle);
    gpath_draw_filled(ctx, s_flairPath);
    gpath_draw_outline(ctx, s_flairPath);
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(tank_bounds.origin.x + 20, tank_bounds.origin.y), GPoint(tank_bounds.size.w - 20, tank_bounds.origin.y) );

  // Halo
  graphics_context_set_stroke_color(ctx, getTrasureColour(s_treasureID));
  graphics_context_set_stroke_width(ctx, 3);
  uint8_t r = 9;
  for (uint8_t h = 0; h < s_haloRings; ++h) {
    graphics_draw_circle(ctx, s_halo, r);
    r +=5;
  }

  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  GRect convBotBound = GRect(4+3 + s_convOffset, 42, 94, 6);
  drawBitmap(ctx, s_convBotBitmap, convBotBound);

  GRect convTopBound = GRect(8+3 - s_convOffset, 30, 94, 12);
  drawBitmap(ctx, s_convTopBitmap, convTopBound);

  GRect convCapBound = GRect(0, 30, 14, 18);
  drawBitmap(ctx, s_convCap, convCapBound);

  drawBitmap(ctx, s_gem, s_treasureRect);
  drawBitmap(ctx, s_sieveBasic, GRect(89, 7, 45, 45));
}

/**
 * Show notify layer. Note that image overlay is done by another gbitmap layer to get transparency
 **/
static void notifyUpdateProc(Layer *this_layer, GContext *ctx) {
  if (s_notifyTreasureID == -1 && s_notifyAchievementID == -1) return; // Nothing to show
  GColor border = GColorBlack;
  static char notifyTxtTop[12]; // Just needs to fit largers of these two below
  const char* notifyTxtBot;
  uint8_t offset = 0;
  GBitmap* image = NULL;
  if (s_notifyTreasureID >= 0) {
    border = getTrasureColour(s_notifyTreasureID);
    strcpy(notifyTxtTop, "Treasure");
    if (s_multipleTreasures) strcpy(notifyTxtTop, " x2");
    strcat(notifyTxtTop, "!");
    notifyTxtBot = ITEM_NAME[s_notifyTreasureID][s_notifyItemID];
    image = getSingleItemImage(s_notifyTreasureID, s_notifyItemID);
    offset = 35;
  } else {
    strcpy(notifyTxtTop, "Achievement!");
    notifyTxtBot = NAME_ACHIEVEMENT[s_notifyAchievementID];
  }
  GRect b = layer_get_bounds(this_layer);
  // Outer box
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, b, 6, GCornersAll);
  graphics_context_set_fill_color(ctx, border);
  graphics_fill_rect(ctx, GRect(b.origin.x+2, b.origin.y+2, b.size.w-4, b.size.h-4), 6, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(b.origin.x+4, b.origin.y+4, b.size.w-8, b.size.h-8), 6, GCornersAll);
  // Text
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, notifyTxtTop, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(b.origin.x+offset, b.origin.y,b.size.w-offset,30), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, notifyTxtBot, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(b.origin.x+offset, b.origin.y+25,b.size.w-offset,30), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  // Image
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  drawBitmap(ctx, image, GRect(b.origin.x+10, b.origin.y+6, 22, 36));
}

bool stopNotify() {
  if (s_notifyTreasureID == -1 && s_notifyAchievementID == -1) return false;
  s_notifyTreasureID = -1;
  s_notifyAchievementID = -1;
  layer_mark_dirty(s_notifyLayer);
  return true;
}

void stopNotifyCallback(void* data) {
  stopNotify();
}

void showNotifyAchievement(uint8_t notifyAchievementID) {
  s_notifyAchievementID = notifyAchievementID;
  app_timer_register(NOTIFY_ACHIEVEMENT_DISPLAY_TIME, stopNotifyCallback, NULL);
  layer_mark_dirty(s_notifyLayer);
}

void showNotifyTreasure(uint8_t treasureID, uint8_t itemID) {
  s_notifyTreasureID = treasureID;
  s_notifyItemID = itemID;
  app_timer_register(NOTIFY_TREASURE_DISPLAY_TIME, stopNotifyCallback, NULL);
  layer_mark_dirty(s_notifyLayer);
}

void create_timeSieve_layer(Window* parentWindow) {
  // Create the clock layer in the top 1/3 of the screen
  Layer* window_layer = window_get_root_layer(parentWindow);
  GRect window_bounds = layer_get_bounds(window_layer);
  GRect layerBounds = GRect(0, window_bounds.size.h/3, window_bounds.size.w, window_bounds.size.h/3);
  s_timeSieveLayer = layer_create( layerBounds );
  // Add as child of the main window layer and set callback
  layer_add_child(window_layer, s_timeSieveLayer);
  layer_set_update_proc(s_timeSieveLayer, timeSieve_update_proc);
  layer_set_clips(s_timeSieveLayer, false);

  //TODO move this into resources
  #ifdef GRAPHICS_ON
  s_convTopBitmap = gbitmap_create_with_resource(RESOURCE_ID_CONV_TOP);
  s_convBotBitmap = gbitmap_create_with_resource(RESOURCE_ID_CONV_BOT);
  s_convCap = gbitmap_create_with_resource(RESOURCE_ID_CONV_CAP);
  s_sieveBasic = gbitmap_create_with_resource(RESOURCE_ID_SIEVE_BASIC);
  #endif

  s_gem = NULL;
  s_treasureRect = GRect(94, 18, 20, 20);

  // Hide halo
  s_haloRings = 0;
  s_treasureID = -1;

  s_flairPath = gpath_create(&FLAIR_PATH);
  gpath_move_to(s_flairPath, GPoint(72, -18));

  // Create layer for the tank
  s_notifyLayer = layer_create( GRect(4, 4, layerBounds.size.w-8, 48) ); // border 4 top and bottom
  layer_set_update_proc(s_notifyLayer, notifyUpdateProc);
  layer_add_child(s_timeSieveLayer, s_notifyLayer);
}

void stopDisplayItem(void* data) {
  if (data != NULL) {
    addItemsMissed(1); // Was a timeout
    APP_LOG(APP_LOG_LEVEL_DEBUG,"ItmMissed!");
    // LEGENDARY BONUS. Give 5% of the current value of the missed item
    if ( getUserItems(LEGENDARY_ID, ITEM_MISSBONUS) == 1 ) {
      addTime( getCurrentSellPrice(s_treasureID, s_itemID) / 20  );
      updateDisplayTime( getUserTime() );
    }
  }
  else app_timer_cancel(s_treasureTimeout);
  s_treasureTimeout = NULL;
  s_treasureRect = GRect(94, 18, 20, 20);
  s_treasureOnShow = false;
  s_treasureID = -1;
  s_haloRings = 0;
}

void itemCanBeCollected() {
  s_treasureOnShow = true;
  // Does it collect itself?
  if (getItemAutoCollect()) {
    collectItem(true);
    return;
  }
  // By sending a (junk) but non-null ptr, we let the callback know it was a timeout, not user intervention
  s_treasureTimeout = app_timer_register(TREASURE_DISPLAY_TIME, stopDisplayItem, (void*)1);   // Start timer to remove treasure - player better be fast!
}

void displyItem(uint8_t treasureID, uint8_t itemID) {
  s_treasureID = treasureID;
  s_itemID = itemID;
  s_gem = getGemImage(treasureID);

  // No animation? pop right in
  if ( getUserOpt(OPT_ANIMATE) == false ) {
    s_treasureRect.origin.x = 45;
    s_halo = GPoint(54,30);
    s_haloRings = 4;
    itemCanBeCollected();
  }
}

bool collectItem(bool autoCollect) {
  if (s_treasureOnShow == false) return false;
  uint8_t nItem = 1;
  s_gem = NULL;

  // LEGENDARY BONUS - sometimes (10%) get two items. Obv not w legendaries
  if ( getUserItems(LEGENDARY_ID, TWOITEM) == 1 ) {
    if (s_treasureID != LEGENDARY_ID && rand()%10 == 0) ++nItem;
  }

  s_multipleTreasures = (nItem > 1);
  addItem(s_treasureID, s_itemID, nItem);
  showNotifyTreasure(s_treasureID, s_itemID);
  stopDisplayItem(NULL);
  if (autoCollect == false) vibes_short_pulse();
  return true;
}

void destroy_timeSieve_layer() {
  layer_destroy(s_timeSieveLayer);

  #ifdef GRAPHICS_ON
  gbitmap_destroy(s_convTopBitmap);
  gbitmap_destroy(s_convBotBitmap);
  gbitmap_destroy(s_convCap);
  gbitmap_destroy(s_sieveBasic);
  #endif

  free(s_flairPath);
}
