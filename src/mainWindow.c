#include <pebble.h>
#include "mainWindow.h"
#include "clock.h"
#include "timeSink.h"
#include "timeTank.h"
#include "timeStore.h"
#include "buyMenu.h"
#include "settingsMenu.h"
#include "sellMenu.h"
#include "persistence.h"
#include "items.h"
#include "achievement.h"
#include "resources.h"
#include "communication.h"
#include "windowManager.h"

void tick_handler(struct tm *tick_time, TimeUnits units_changed); //rm me later

static TimeUnits s_units_changed; // Used in anim

// Hold if the animation routine is still in progress and is requesting more frames.
static bool s_clockAnimRequest;
static bool s_sinkAnimRequest;
static bool s_tankAnimRequest;

void animEnd() {
    updateDisplayTime( getUserTime() ); // Make sure correct time is set
    update_timeTank_layer(); // Show the user their new time
    updateClockLayer();  // Update the clock
    updateBuyLayer();
    updateSellLayer();
    updateSettingsLayer();
}

/**
 * Function executes at 1/FPS while all areas are still requesting more frames.
 * Each anim function is expected to dirty any layers which need redrawing
 */
void animCallback(void* data) {
  if (s_tankAnimRequest)  s_tankAnimRequest  = tankAnimCallback(s_units_changed);
  if (s_sinkAnimRequest)  s_sinkAnimRequest  = sinkAnimCallback(s_units_changed);
  if (s_clockAnimRequest) s_clockAnimRequest = clockAnimCallback(s_units_changed);
  if (s_clockAnimRequest || s_tankAnimRequest || s_clockAnimRequest) {
    app_timer_register(ANIM_DELAY, animCallback, NULL);
  } else {
    animEnd();
  }
}

/**
 * Call to start the per-min animation. Resets all amination and starts the callback.
 */
void animBegin() {
  clockAnimReset(s_units_changed);
  sinkAnimReset(s_units_changed);
  tankAnimReset(s_units_changed);
  s_clockAnimRequest = true;
  s_sinkAnimRequest = true;
  s_tankAnimRequest = true;
  animCallback(NULL);
}

TimeUnits getLastTimeUnit() {
  return s_units_changed;
}

// Main window initialisation
void mainWindowLoad(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "mainW load");
  create_timeSink_layer(window);
  create_clock_layer(window);
  create_timeTank_layer(window);
  initMainWindowRes();
  update_tick_handler(); // Get 1s or 1m callbacks
  registerCommunication();
  accel_tap_service_subscribe( (AccelTapHandler) tapHandle );
  APP_LOG(APP_LOG_LEVEL_DEBUG, "mainW load done");
}

// Main window destructiom
void mainWindowUnload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "mainW Uload");
  destroy_timeSink_layer();
  destroy_clock_layer();
  destroy_timeTank_layer();
  deinitMainWindowRes();
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "mainW Uload done");
}

// For the main window
void main_window_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  // collectItem takes if this is an auto-collect. As this is button-based the answer is no
  if (collectItem(false) == true) return; // First see if there is an item to collect
  if (stopNotify() == true) return; // Second see if there is a notify window to dismiss

  if (BUTTON_ID_UP == button) {
    pushWindow(WINDOW_BUY, true);
  } else if (BUTTON_ID_SELECT == button) {
    pushWindow(WINDOW_SETTINGS, true);
  } else if (BUTTON_ID_DOWN == button) {
    pushWindow(WINDOW_SELL, true);
  }
}

void mainWindowClickConfigProvider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_DOWN, main_window_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, main_window_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, main_window_single_click_handler);
  if (IS_DEBUG) window_single_click_subscribe(BUTTON_ID_BACK, main_window_single_click_handler);
}

/**
 * Called on every tap. We only use this to collect items manually (the passed false)
 **/
void tapHandle(AccelData *data, uint32_t num_samples) {
  if (collectItem(false) == true) return; // First see if there is an item to collect
  if (stopNotify() == true) return; // Second see if there is a notify window to dismiss
}

void doSecond() {
  // Update the user time, but only 1/60 of it (we give the remainder on the min mark)
  addTime( getTimePerMin() / SEC_IN_MIN );
  updateDisplayTime( getUserTime() ); // We don't animate the time if updating every sec
  update_timeTank_layer();
  updateClockLayer();
  updateBuyLayer();
  updateSellLayer();
  updateSettingsLayer();
}

/**
 * The Tick Handler is called once per second or once per min, depending on the
 * user's preferences. It is the main instigator of activity.
 * Here we:
 * Reward the user with more time
 * Run the basic market sim to update sell prices
 * Check to see if the user found an item
 * Check if any achievements are earned (note does not run if item found)
 * Check to see if we should notfiy the user about finding an item/chevo
 * Finally, if animate is true we insitigate the animation routine which is self-
 * sustaining until all animations have played out
 **/
void tick_handler(struct tm* tick_time, TimeUnits units_changed) {

  if (getUserOpt(OPT_SHOW_SECONDS) == true) doSecond();
  updateTimeBuffer();

  if ((units_changed & MINUTE_UNIT) == 0) return; // If no min change then stop here
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MIN TICK"); memRep(NULL);

  // Change of day?
  if ((units_changed & DAY_UNIT) > 0) updateDateBuffer();

  // For everthing else, we take midday as a new 'day' as well
  if (tick_time->tm_hour == 12 && tick_time->tm_min == 0) {
    units_changed = units_changed | DAY_UNIT; // Add day unit
    // Remove second unit - this is used internally to tell true day boundaries from midday ones
    units_changed = units_changed & (MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT);
  }

  s_units_changed = units_changed;
  if (getUserOpt(OPT_SHOW_SECONDS) == true) { // If we are doing per-sec updates then give the modulo remainder time
    addTime( getTimePerMin() % SEC_IN_MIN );
  } else { // Give the whole 1m allowance
    addTime( getTimePerMin() );
  }

  // LEGENDARY BONUS - collect 3x the time on the hour
  if ( getUserItems(LEGENDARY_ID, TPM_3HOURBONUS) == 1 ) {
    addTime( getTimePerMin() * 3 );
  }

  // Update prices
  modulateSellPrices();

  // Found treasure? -1 for no
  int8_t itemFoundQuality = checkForItem( units_changed );

  // Only get achievements if we did not find treasure? -1 for no
  int8_t achievementEarned = -1;
  if (itemFoundQuality == -1) achievementEarned = checkAchievements();

  // Notify user
  if (itemFoundQuality >= 0 || achievementEarned >= 0) {
    uint8_t light = getUserSetting(SETTING_LIGHT);
    uint8_t vibe = getUserSetting(SETTING_VIBE);
    uint16_t minsOfDay = (tick_time->tm_hour * 60) + tick_time->tm_min;
    bool zzzTime = false;
    if ( getUserSetting(SETTING_ZZZ_END) >= getUserSetting(SETTING_ZZZ_START)) {
      // Case that start and end are within one day
      if ( minsOfDay > getUserSetting(SETTING_ZZZ_START)*60 && minsOfDay < getUserSetting(SETTING_ZZZ_END)*60) {
        zzzTime = true;
      }
    } else {
      // Case that start and end are on different days
      if ( minsOfDay > getUserSetting(SETTING_ZZZ_START)*60 || minsOfDay < getUserSetting(SETTING_ZZZ_END)*60) {
        zzzTime = true;
      }
    }
    // Remember a setting of zero means no notify - so we need to add one here (as zero is also common item quality)
    if (light > 0 && !zzzTime && itemFoundQuality+1 >= light) light_enable_interaction();
    if (vibe  > 0 && !zzzTime && itemFoundQuality+1 >= vibe ) vibes_double_pulse();
    if (!zzzTime && achievementEarned >= 0) { // No settings to control these ones, but they're rare
      light_enable_interaction();
      vibes_long_pulse();
    }
  }

  // Begin animation?
  if (getUserOpt(OPT_ANIMATE) == true) { // do animation?
    animBegin();
  } else {
    animEnd(); // Just redraw
  }

  // Save on the hour, and push this save to the phone
  if ((units_changed & HOUR_UNIT) > 0) {
    saveState();
    sendStateToPhone();
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "TICK_E"); memRep(NULL);
}

void update_tick_handler() {
  if (getUserOpt(OPT_SHOW_SECONDS) == true) {
    tick_timer_service_subscribe(SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT, tick_handler);
  }
  updateClockLayer();
}
