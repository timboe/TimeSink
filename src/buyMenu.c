#include <pebble.h>
#include "buyMenu.h"
#include "buyMenuSub.h"
#include "constants.h"

// Menu layers for my window
static MenuLayer* s_menuLayer = NULL;

// My sub-windows
static Window* s_refineryWindow;
static Window* s_tankWindow;
static Window* s_watcherWindow;

static int s_refineryContext = REFINERY_ID;
static int s_tankContext = TANK_ID;
static int s_watcherContext = WATCHER_ID;

void updateBuyLayer() {
  if (s_menuLayer != NULL) layer_mark_dirty(menu_layer_get_layer(s_menuLayer));
  updateBuyMenuSubLayer();
}

///
/// MAIN BUY WINDOW CALLBACKS
///

static uint16_t buyMenuGetNumSectionsCallback(MenuLayer* menu_layer, void* data) { return NUM_BUY_MENU_SECTIONS; }

static uint16_t buyMenuGetNumRowsCallback(MenuLayer* menu_layer, uint16_t section_index, void* data) { return NUM_BUY_MENU_ITEMS; }

static int16_t buyMenuGetHeaderHeightCallback(MenuLayer* menu_layer, uint16_t section_index, void* data) { return MENU_CELL_BASIC_HEADER_HEIGHT; }

static void buyMenuDrawHeaderCallback(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* data) { menu_cell_basic_header_draw(ctx, cell_layer, "BUY Upgrades"); }

static void buyMenuDrawRowCallback(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* data) {
  // Determine which section we're going to draw in
  const GSize size = layer_get_frame(cell_layer).size;
  const bool selected = menu_cell_layer_is_highlighted(cell_layer);
  const int row = cell_index->row;
  if (selected)        graphics_context_set_fill_color(ctx, MENU_BACK_GREEN_SELECT);
  else if (row%2 == 1) graphics_context_set_fill_color(ctx, MENU_BACK_GREEN_ODD);
  else                 graphics_context_set_fill_color(ctx, MENU_BACK_GREEN_EVEN);
  graphics_fill_rect(ctx, GRect(0, 0, size.w, size.h), 0, GCornersAll);
  if (row == REFINERY_ID) {
    menu_cell_basic_draw(ctx, cell_layer, "ADDONS", "Get more liquid time", NULL);
  } else if (row == TANK_ID) {
    menu_cell_basic_draw(ctx, cell_layer, "TANKS", "Store more liquid time", NULL);
  } else if (row == WATCHER_ID) {
    menu_cell_basic_draw(ctx, cell_layer, "EMPLOYEES", "They work for YOU", NULL);
  }
  graphics_draw_line(ctx, GPoint(0,0), GPoint(size.w, 0) );
}

static void buyMenuSelectCallback(MenuLayer* menu_layer, MenuIndex* cell_index, void* data) {
  // Use the row to specify which item will receive the select action
  const int row = cell_index->row;
  if (row == REFINERY_ID) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "PUSH REFINERY");
    window_stack_push(s_refineryWindow, true);
  } else if (row == TANK_ID) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "PUSH TANK");
    window_stack_push(s_tankWindow, true);
  } else if (row == WATCHER_ID) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "PUSH WATCHER");
    window_stack_push(s_watcherWindow, true);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void buy_window_load(Window* parentWindow) {

  // Create the menu layer
  s_menuLayer = menu_layer_create(layer_get_frame(window_get_root_layer(parentWindow)));
  menu_layer_set_callbacks(s_menuLayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = buyMenuGetNumSectionsCallback,
    .get_num_rows = buyMenuGetNumRowsCallback,
    .get_header_height = buyMenuGetHeaderHeightCallback,
    .draw_header = buyMenuDrawHeaderCallback,
    .draw_row = buyMenuDrawRowCallback,
    .select_click = buyMenuSelectCallback,
  });
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menuLayer, parentWindow);
  menu_layer_set_normal_colors(s_menuLayer, MENU_BACK_GREEN_ODD, GColorBlack);
  layer_add_child(window_get_root_layer(parentWindow), menu_layer_get_layer(s_menuLayer));

  // Setup sub-windows that we might want to jump to
  createSubWin(&s_refineryWindow, &s_refineryContext);
  createSubWin(&s_tankWindow, &s_tankContext);
  createSubWin(&s_watcherWindow, &s_watcherContext);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"BUY WIN LOAD");

}

void buy_window_unload() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"BUY WIN DESTROY");

  // Destroy sub-windows
  window_destroy(s_refineryWindow);
  window_destroy(s_tankWindow);
  window_destroy(s_watcherWindow);
  s_refineryWindow = NULL;
  s_tankWindow = NULL;
  s_watcherWindow = NULL;

  menu_layer_destroy(s_menuLayer);
  s_menuLayer = NULL;
}
