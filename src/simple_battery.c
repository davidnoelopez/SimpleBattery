#include "pebble.h"
// sample change
Window *window;
Layer *date_layer;
Layer *time_layer;
Layer *seconds_layer;
Layer *line_layer;
Layer *battery_layer;

TextLayer *text_date_layer;
TextLayer *text_time_layer;
int seconds = 0;
int day = 0;
int battery_percent = 0;

void line_layer_update(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void battery_layer_update(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 3, 144, 1), 0, GCornerNone);
  int width = (int)(1.44 * battery_percent);
  GRect battery_line_frame = GRect(0, 0, width, 2);
  graphics_fill_rect(ctx, battery_line_frame, 0, GCornerNone);
}

void seconds_layer_update(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  GRect bounds = layer_get_bounds(layer);
  int width = (int)(seconds * 2.4);
  bounds.size.w = width;
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void date_layer_update(Layer *layer, GContext* ctx) {
  static char date_text[] = "XXX 00";
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  strftime(date_text, sizeof(date_text), "%b %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);
}

void time_layer_update(Layer *layer, GContext* ctx) {
  static char time_text[] = "00:00";
  char *time_format;

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(time_text, sizeof(time_text), time_format, tick_time);
  text_layer_set_text(text_time_layer, time_text);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {  
  if(!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
  }
  seconds = tick_time->tm_sec;

  layer_mark_dirty(seconds_layer);
  if (seconds == 0) {
    layer_mark_dirty(time_layer);
  }

  if (day != tick_time->tm_mday) {
    day = tick_time->tm_mday;
    layer_mark_dirty(date_layer);
  }
}

static void handle_battery(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  layer_mark_dirty(battery_layer);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);

  layer_destroy(line_layer);
  layer_destroy(battery_layer);
  layer_destroy(date_layer);
  layer_destroy(time_layer);
  layer_destroy(seconds_layer);

  window_destroy(window);
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);

  // Init date layer
  date_layer = layer_create(GRect(8, 44, 136, 35));
  layer_set_update_proc(date_layer, date_layer_update);
  layer_add_child(window_layer, date_layer);

  // Init date text layer
  text_date_layer = text_layer_create(layer_get_bounds(date_layer));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  GFont fontDate = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OSP_DIN_35));
  text_layer_set_font(text_date_layer, fontDate);
  layer_add_child(date_layer, text_layer_get_layer(text_date_layer));

  // Init time layer
  time_layer = layer_create(GRect(8, 79, 136, 44));
  layer_set_update_proc(time_layer, time_layer_update);
  layer_add_child(window_layer, time_layer);

  // Init time text layer
  text_time_layer = text_layer_create(layer_get_bounds(time_layer));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  GFont fontTime = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OSP_DIN_44));
  text_layer_set_font(text_time_layer, fontTime);
  layer_add_child(time_layer, text_layer_get_layer(text_time_layer));

  // Init center line
  GRect line_frame = GRect(8, 84, 136, 3);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_update);
  layer_add_child(window_layer, line_layer);

  // Init battery status line
  BatteryChargeState charge_state = battery_state_service_peek();
  battery_percent = charge_state.charge_percent;
  GRect battery_line_frame = GRect(0, 0, 144, 4);
  battery_layer = layer_create(battery_line_frame);
  layer_set_update_proc(battery_layer, battery_layer_update);
  layer_add_child(window_layer, battery_layer);

  // Init seconds line
  seconds_layer = layer_create(GRect(0, 165, 144, 3));
  layer_set_update_proc(seconds_layer, seconds_layer_update);
  layer_add_child(window_layer, seconds_layer);

  battery_state_service_subscribe(&handle_battery);
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  handle_second_tick(NULL, SECOND_UNIT);
}


int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
