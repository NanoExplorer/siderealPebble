#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;
static double longitude = -22856.471;
static const uint32_t LONGITUDE = 1;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void recieve_location_info(DictionaryIterator *iterator, void *context){
  Tuple *dataln = dict_find(iterator, LONGITUDE);
  
  
  if(dataln) {
    longitude = ((float)((int) dataln -> value -> int32))/100;
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "App message does not contain longitude");
  }
  if(dataln){
    text_layer_set_text(s_connection_layer, "up-to-date");
    persist_write_int(LONGITUDE, (int)(longitude*100));
  } else {
    text_layer_set_text(s_connection_layer, "sync failed");
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done recieving!");
  
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  // Needs to be static because it's used by the system later.
  static char s_time_text[] = "00:00:00";
  static char s_minute[] = ":00";
  static char s_sec[] = ":00";
  time_t unixtime = mktime(tick_time);
  
  double sidereal_factor = 1.0027379093507949;
  double sidereal_offset = 67310.54840880001;
  //longitude = -22856.471;
  //        This number is the conversion from unix epoch to reduced julian date epoch  v
  int siderealTime = (int)(sidereal_offset + sidereal_factor * ((double)(unixtime) - 946728000) + longitude);
  //static char debug_text[] = "0000000000";
  //snprintf(debug_text,sizeof(debug_text),"%d",(int)(longitude*100));
  //APP_LOG(APP_LOG_LEVEL_DEBUG, debug_text);
  int siderealHr = (siderealTime % 86400)  / 3600;
  int siderealMin = ( siderealTime % 3600 ) / 60;
  int siderealSec = (siderealTime % 60);
  //strftime(s_time_text, sizeof(s_time_text), "%T", tick_time);
  if (siderealMin < 10) {
    snprintf(s_minute,sizeof(s_minute),":0%d",siderealMin);
  } else {
    snprintf(s_minute,sizeof(s_minute),":%d",siderealMin);
  } 
  
  if (siderealSec < 10) {
    snprintf(s_sec,sizeof(s_sec),":0%d",siderealSec);
  } else {
    snprintf(s_sec,sizeof(s_sec),":%d",siderealSec);
  }
  snprintf(s_time_text,sizeof(s_time_text),"%d",siderealHr);
  strcat(s_time_text,s_minute);
  strcat(s_time_text,s_sec);
  text_layer_set_text(s_time_layer, s_time_text);

  handle_battery(battery_state_service_peek());
}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_connection_layer, connected ? "connected" : "disconnected");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_time_layer = text_layer_create(GRect(0, 40, bounds.size.w, 34));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  s_connection_layer = text_layer_create(GRect(0, 90, bounds.size.w, 34));
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentCenter);
  handle_bluetooth(bluetooth_connection_service_peek());

  s_battery_layer = text_layer_create(GRect(0, 120, bounds.size.w, 34));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_text(s_battery_layer, "100% charged");

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  battery_state_service_subscribe(handle_battery);
  bluetooth_connection_service_subscribe(handle_bluetooth);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
}

static void init() {
  if(persist_exists(LONGITUDE)){
    longitude = ((double)(persist_read_int(LONGITUDE))) / 100;
  }
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(recieve_location_info);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
  app_message_deregister_callbacks();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
