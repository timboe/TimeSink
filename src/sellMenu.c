#include <pebble.h>
#include "sellMenu.h"
#include "constants.h"
#include "persistence.h"
#include "timeStore.h"
#include "resources.h"
#include "items.h"
#include "notify.h"

// Box is 32x27, ~14 px vert. available
static const GPathInfo ARROW_DOWN_PATH = {
  .num_points = 7,
  .points = (GPoint []) {{0, 7}, {10, 0}, {3, 0}, {3, -7}, {-3, -7}, {-3, 0}, {-10, 0}}
};

static const GPathInfo ARROW_UP_PATH = {
  .num_points = 7,
  .points = (GPoint []) {{0, -7}, {10, 0}, {3, 0}, {3, 7}, {-3, 7}, {-3, 0}, {-10, 0}}
};

static const GPathInfo BOX_PATH = {
  .num_points = 4,
  .points = (GPoint []) {{0, 7}, {7, 0}, {0, -7}, {-7, 0}}
};

static GPath* s_arrowUp;
static GPath* s_arrowDown;
static GPath* s_box;

static int8_t s_sellSections[SELLABLE_CATEGORIES] = {-1};

static MenuLayer* s_sellLayer;

// Hold text of top sell notify
static char soldTextTop[TEXT_BUFFER_SIZE];
static char soldTextBot[TEXT_BUFFER_SIZE];

void updateSellLayer() {
  if (s_sellLayer) layer_mark_dirty(menu_layer_get_layer(s_sellLayer));
}

int8_t getItemIDFromRow(const unsigned treasureID, const uint16_t row) {
  uint8_t hasItems = 0;
  uint8_t max = MAX_TREASURES;
  if (treasureID == LEGENDARY_ID) max = MAX_UNIQUE;
  // Find the row'th item
  for (uint8_t itemID = 0; itemID < max; ++itemID) {
    if (getUserItems(treasureID, itemID) > 0 && hasItems++ == row) return itemID;
  }
  return -1;
}

int8_t getAchievementIDFromRow(const uint16_t row) {
  uint8_t hasChevos = 0;
  for (uint8_t chevoID = 0; chevoID < MAX_CHEVOS; ++chevoID) {
    if (getUserChevo(chevoID) && hasChevos++ == row) return chevoID;
  }
  return -1;
}

///
/// SELL WINDOW CALLBACKS
///

/**
 * One (empty) section at the top for the heade, plus one section for each category of item the player ownes
 */
static uint16_t sell_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  uint16_t sections = 0;
  for (uint16_t i = 0; i < SELLABLE_CATEGORIES; ++i) {
    if (getUserTotalItems(i) > 0) s_sellSections[sections++] = i; // Note - postincrement
  }
  if (sections == 0) {
    s_sellSections[0] = -1;
    return 1; // Generic title
  }
  return sections;
}

static uint16_t sell_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if (s_sellSections[section_index] == -1) return 1; // If no items to sell
  return getUserItemTypes( s_sellSections[section_index] );
}

static int16_t sell_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0: return 2*MENU_CELL_BASIC_HEADER_HEIGHT;
    default: return MENU_CELL_BASIC_HEADER_HEIGHT;
  }
}

static void sell_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  const GSize size = layer_get_frame(cell_layer).size;
  GRect top = GRect(0, 0, size.w, MENU_CELL_BASIC_HEADER_HEIGHT);
  GRect bot = GRect(0, MENU_CELL_BASIC_HEADER_HEIGHT, size.w, MENU_CELL_BASIC_HEADER_HEIGHT);

  if (section_index == 0) {
    graphics_draw_text(ctx, "SELL Items", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), top, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    if (s_sellSections[0] == -1) {
      graphics_draw_text(ctx, "You have no items!", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), bot, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
      return;
    }
    top = bot;
  }

  const int8_t category = s_sellSections[section_index];

  strcpy(getTempBuffer(), ITEM_CATEGORIE_NAME[category]);
  strcat(getTempBuffer(), " Treasures");

  GColor backColor;
  if      (category == COMMON_ID) backColor = GColorLightGray;
  else if (category == MAGIC_ID)  backColor = MENU_BACK_GREEN_ODD;
  else if (category == RARE_ID)   backColor = MENU_BACK_BLUE_ODD;
  else if (category == EPIC_ID)   backColor = MENU_BACK_PURPLE_ODD;

  graphics_context_set_fill_color(ctx, backColor);
  graphics_fill_rect(ctx, top, 0, GCornersAll);
  graphics_draw_text(ctx, getTempBuffer(), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), top, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static int16_t sell_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return MENU_TWO_CELL_HEIGHT;
}

static void sell_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {

  const GSize size = layer_get_frame(cell_layer).size;
  const uint16_t section = cell_index->section;
  const uint16_t row = cell_index->row;
  const bool selected = menu_cell_layer_is_highlighted(cell_layer);

  // get type
  if (section == 0 && s_sellSections[0] == -1) {
    // just draw a background coloured box and return as there is nothing to sell
    return;
  }

  const int8_t treasureID = s_sellSections[section];
  int8_t itemID = getItemIDFromRow(treasureID, row);
  // Find the row'th item

  if (itemID == -1) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "NoItem4SelRow");
    return;
  }

  GColor backColor;
  if (treasureID == COMMON_ID) {
    if (selected) backColor = GColorDarkGray;
    else backColor = GColorLightGray;
  } else if (treasureID == MAGIC_ID) {
    if (selected) backColor = MENU_BACK_GREEN_SELECT;
    else if (row%2==0) backColor = MENU_BACK_GREEN_EVEN;
    else backColor = MENU_BACK_GREEN_ODD;
  } else if (treasureID == RARE_ID) {
    if (selected) backColor = MENU_BACK_BLUE_SELECT;
    else if (row%2==0) backColor = MENU_BACK_BLUE_EVEN;
    else backColor = MENU_BACK_BLUE_ODD;
  } else if (treasureID == EPIC_ID) {
    if (selected) backColor = MENU_BACK_PURPLE_SELECT;
    else if (row%2==0) backColor = MENU_BACK_PURPLE_EVEN;
    else backColor = MENU_BACK_PURPLE_ODD;
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "SlMnu Unwn TsrId");
  }
  graphics_context_set_fill_color(ctx, backColor);
  graphics_fill_rect(ctx, GRect(0, 0, size.w, size.h), 0, GCornersAll);

  if (selected) graphics_context_set_text_color(ctx, GColorWhite);
  else graphics_context_set_text_color(ctx, GColorBlack);

  static char subText1[TEXT_BUFFER_SIZE];
  static char subText2[TEXT_BUFFER_SIZE];
  strcpy(subText1, "");

  GRect ttlTextRect = GRect(MENU_X_OFFSET, -6, size.w-MENU_X_OFFSET, size.h);
  GRect topTextRect = GRect(MENU_X_OFFSET, 16, size.w-MENU_X_OFFSET, size.h-22);
  GRect medTextRect = GRect(MENU_X_OFFSET, 27, size.w-MENU_X_OFFSET, size.h-33);

  // Get owned
  uint16_t owned = getUserItems(treasureID, itemID);
  // Get sell price
  uint64_t sellPrice = owned * getCurrentSellPrice(treasureID, itemID);
  snprintf(subText1, TEXT_BUFFER_SIZE, "OWNED: %i", (int)owned);
  strcpy(subText2, "VALUE:");
  timeToString(sellPrice, getTempBuffer(), TEXT_BUFFER_SIZE, true);
  strcat(subText2, getTempBuffer());

  graphics_draw_text(ctx, ITEM_NAME[treasureID][itemID], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), ttlTextRect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, subText1, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), topTextRect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, subText2, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), medTextRect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  // market box
  unsigned percentage;
  currentSellPricePercentage(getTempBuffer(), TEXT_BUFFER_SIZE, &percentage, treasureID, itemID);
  GColor percBoxBack;
  GColor percBoxFront;
  GColor percBoxHighlight;
  GPath* arrow;
  if (percentage > 110) {
    arrow = s_arrowUp;
    percBoxHighlight = GColorDarkGreen;
    if (selected) {
      percBoxBack = GColorJaegerGreen;
      percBoxFront = GColorBrightGreen;
    } else {
      percBoxBack = GColorBrightGreen;
      percBoxFront = GColorJaegerGreen;
    }
  } else if (percentage < 90) {
    arrow = s_arrowDown;
    percBoxHighlight = GColorBulgarianRose;
    if (selected) {
      percBoxBack = GColorRed;
      percBoxFront = GColorMelon;
    } else {
      percBoxBack = GColorMelon;
      percBoxFront = GColorRed;
    }
  } else {
    arrow = s_box;
    percBoxHighlight = GColorOxfordBlue;
    if (selected) {
      percBoxBack = GColorCobaltBlue;
      percBoxFront = GColorCeleste;
    } else {
      percBoxBack = GColorCeleste;
      percBoxFront = GColorCobaltBlue;
    }
  }
  GRect valueBox = GRect(110, 2, 32, 27);
  GRect valueBorder = GRect(109, 1, 34, 29);
  GRect percBox = GRect(110, 14, 32, 10);
  graphics_context_set_stroke_color(ctx, percBoxHighlight);
  graphics_context_set_fill_color(ctx, percBoxHighlight);
  graphics_fill_rect(ctx, valueBorder, 2, GCornersAll);
  graphics_context_set_fill_color(ctx, percBoxBack);
  graphics_fill_rect(ctx, valueBox, 2, GCornersAll);
  graphics_draw_text(ctx, getTempBuffer(), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), percBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_context_set_fill_color(ctx, percBoxFront);
  gpath_draw_filled(ctx, arrow);
  gpath_draw_outline(ctx, arrow);

  // Image
  GRect imageRect = GRect(3, 4,  22, 36);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  drawBitmap(ctx, getSellItemImage(treasureID, itemID), imageRect);
}

void doSell(const uint16_t section, const uint16_t row, bool sellAll) {
  // get type
  if (section == 0 && s_sellSections[0] == -1) return;

  const int8_t treasureID = s_sellSections[section];
  int8_t itemID = getItemIDFromRow(treasureID, row);
  if (itemID == -1) return;

  uint16_t sold = sellItem(treasureID, itemID, sellAll);
  GColor highlight = GColorBlack;

  snprintf(soldTextTop, TEXT_BUFFER_SIZE, "Sold %i", (int)sold);
  if (sold == 0) {
    strcpy(soldTextBot, "Time Tank Too Small!");
    highlight = GColorRed;
  } else {
    timeToString(sold * getCurrentSellPrice(treasureID, itemID), getTempBuffer(), TEXT_BUFFER_SIZE, true);
    strcpy(soldTextBot, "For ");
    strcat(soldTextBot, getTempBuffer());
  }

  showNotify(highlight, soldTextTop, ITEM_NAME[treasureID][itemID], soldTextBot);
}

static void sell_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SAll" );
  doSell(cell_index->section, cell_index->row, true);
  layer_mark_dirty(menu_layer_get_layer(menu_layer));
}

static void sell_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SOne" );
  doSell(cell_index->section, cell_index->row, false);
  layer_mark_dirty(menu_layer_get_layer(menu_layer));
}

///
/// SETUP
///

void sellWindowLoad(Window* parentWindow) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"SelWinLd");

  initSellWindowRes();

  // Now we prepare to initialize the menu layer
  Layer* windowLayer = window_get_root_layer(parentWindow);
  GRect bounds = layer_get_frame(windowLayer);

  s_arrowUp = gpath_create(&ARROW_UP_PATH);
  s_arrowDown = gpath_create(&ARROW_DOWN_PATH);
  s_box = gpath_create(&BOX_PATH);
  gpath_move_to(s_arrowUp, GPoint(126, 10));
  gpath_move_to(s_arrowDown, GPoint(126, 10));
  gpath_move_to(s_box, GPoint(126, 10));

  // Create the menu layer
  s_sellLayer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_sellLayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = sell_get_num_sections_callback,
    .get_num_rows = sell_get_num_rows_callback,
    .get_cell_height = sell_get_cell_height_callback,
    .get_header_height = sell_get_header_height_callback,
    .draw_header = sell_draw_header_callback,
    .draw_row = sell_draw_row_callback,
    .select_click = sell_select_callback,
    .select_long_click = sell_select_long_callback,
  });
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_normal_colors(s_sellLayer, MENU_BACK_RED_ODD, GColorBlack);
  menu_layer_set_highlight_colors(s_sellLayer, MENU_BACK_RED_ODD, MENU_BACK_RED_ODD);
  menu_layer_set_click_config_onto_window(s_sellLayer, parentWindow);
  layer_add_child(windowLayer, menu_layer_get_layer(s_sellLayer));

  // Notify layer goes on top, shows sold items
  layer_add_child(windowLayer, getNotifyLayer());

}

void sellWindowUnload() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"SelWinULd");
  menu_layer_destroy(s_sellLayer);
  destroyNotifyLayer();
  free(s_arrowUp);
  free(s_arrowDown);
  free(s_box);
  deinitSellWindowRes();
}
