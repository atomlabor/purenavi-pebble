#include <pebble.h>

#ifndef MESSAGE_KEY_ID_VIBE
#define MESSAGE_KEY_ID_VIBE 7
#endif
#ifndef MESSAGE_KEY_ID_ACCURACY
#define MESSAGE_KEY_ID_ACCURACY 8
#endif

static Window *s_main_window;
static Layer *s_arrow_layer, *s_sidebar_layer;
static TextLayer *s_dist_layer, *s_time_layer, *s_battery_layer, *s_acc_layer, *s_dir_text_layer, *s_hint_layer;
static TextLayer *s_label_s, *s_label_i, *s_label_p;

static char s_buffer[128] = "", s_time_buffer[8], s_batt_buffer[8], s_acc_buffer[12];
static int32_t s_target_bearing = 0, s_gps_heading = -1, s_current_arrow_angle = 0, s_last_arrow_angle = 0;
static bool s_gps_fixed = false, s_inverted = true; 
static GPath *s_arrow_path = NULL;
static const GPathInfo ARROW_POINTS = { .num_points = 4, .points = (GPoint []) { {0, -45}, {22, 35}, {0, 15}, {-22, 35} } };

static int s_sw; 

static int32_t normalize_angle(int32_t angle) {
  while (angle < 0) angle += TRIG_MAX_ANGLE;
  while (angle >= TRIG_MAX_ANGLE) angle -= TRIG_MAX_ANGLE;
  return angle;
}

static void handle_battery(BatteryChargeState charge_state) {
  snprintf(s_batt_buffer, sizeof(s_batt_buffer), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_batt_buffer);
}

static void update_time() {
  time_t temp = time(NULL); struct tm *t = localtime(&temp);
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void update_ui_colors() {
  GColor fg = s_inverted ? GColorWhite : GColorBlack;
  GColor bg_side = s_inverted ? GColorWhite : GColorBlack;
  GColor fg_side = s_inverted ? GColorBlack : GColorWhite;
  
  text_layer_set_text_color(s_dist_layer, fg); text_layer_set_text_color(s_time_layer, fg);
  text_layer_set_text_color(s_battery_layer, fg); text_layer_set_text_color(s_dir_text_layer, fg);
  text_layer_set_text_color(s_hint_layer, fg); text_layer_set_text_color(s_acc_layer, fg);
  
  if (s_sw > 0) {
    text_layer_set_text_color(s_label_s, fg_side); text_layer_set_background_color(s_label_s, bg_side);
    text_layer_set_text_color(s_label_i, fg_side); text_layer_set_background_color(s_label_i, bg_side);
    text_layer_set_text_color(s_label_p, fg_side); text_layer_set_background_color(s_label_p, bg_side);
  }
  
  layer_mark_dirty(s_arrow_layer); 
  layer_mark_dirty(s_sidebar_layer);
}

static void arrow_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_inverted ? GColorBlack : GColorWhite);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  
  if (s_gps_fixed) {
    layer_set_hidden(text_layer_get_layer(s_hint_layer), true);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, (s_inverted ? GColorWhite : GColorBlack)));
    
    int32_t diff = normalize_angle(s_current_arrow_angle - s_last_arrow_angle);
    if (diff > 32768) diff -= 65536;
    s_last_arrow_angle = normalize_angle(s_last_arrow_angle + diff / 4);
    
    gpath_rotate_to(s_arrow_path, s_last_arrow_angle);
    gpath_move_to(s_arrow_path, GPoint(b.size.w / 2, b.size.h / 2));
    gpath_draw_filled(ctx, s_arrow_path);
  } else {
    layer_set_hidden(text_layer_get_layer(s_hint_layer), false);
  }
}

static void sidebar_update_proc(Layer *layer, GContext *ctx) {
  if (s_sw <= 0) return;
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_inverted ? GColorWhite : GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, s_inverted ? GColorBlack : GColorWhite);
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(0, b.size.h));
  graphics_draw_line(ctx, GPoint(0, b.size.h / 3), GPoint(s_sw, b.size.h / 3));
  graphics_draw_line(ctx, GPoint(0, (2 * b.size.h) / 3), GPoint(s_sw, (2 * b.size.h) / 3));
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t_dist = dict_find(iter, MESSAGE_KEY_ID_DIST);
  if (t_dist) { s_gps_fixed = true; strncpy(s_buffer, t_dist->value->cstring, sizeof(s_buffer) - 1); text_layer_set_text(s_dist_layer, s_buffer); }
  Tuple *t_bear = dict_find(iter, MESSAGE_KEY_ID_BEARING); if (t_bear) s_target_bearing = t_bear->value->int32;
  Tuple *t_head = dict_find(iter, MESSAGE_KEY_ID_GPS_HEADING); if (t_head) s_gps_heading = t_head->value->int32;
  Tuple *t_acc = dict_find(iter, MESSAGE_KEY_ID_ACCURACY);
  if (t_acc) {
    snprintf(s_acc_buffer, sizeof(s_acc_buffer), "+/-%dm", (int)t_acc->value->int32);
    text_layer_set_text(s_acc_layer, s_acc_buffer);
  }
  if (dict_find(iter, MESSAGE_KEY_ID_VIBE)) vibes_double_pulse();
  layer_mark_dirty(s_arrow_layer);
}

static void compass_handler(CompassHeadingData heading) {
  if (heading.compass_status == CompassStatusDataInvalid) return;
  int32_t dev_head = (s_gps_heading >= 0) ? (s_gps_heading * TRIG_MAX_ANGLE / 360) : (int32_t)heading.true_heading;
  s_current_arrow_angle = normalize_angle(s_target_bearing * TRIG_MAX_ANGLE / 360 - dev_head);
  
  int degrees = (int)(((int64_t)s_current_arrow_angle * 360) / 65536);
  if (degrees > 180) degrees -= 360;
  if (s_gps_fixed) {
    if (degrees >= -22 && degrees <= 22) text_layer_set_text(s_dir_text_layer, "GO STRAIGHT");
    else if (degrees > 22 && degrees <= 65) text_layer_set_text(s_dir_text_layer, "SLIGHT RIGHT");
    else if (degrees > 65 && degrees <= 115) text_layer_set_text(s_dir_text_layer, "TURN RIGHT");
    else if (degrees < -22 && degrees >= -65) text_layer_set_text(s_dir_text_layer, "SLIGHT LEFT");
    else if (degrees < -65 && degrees >= -115) text_layer_set_text(s_dir_text_layer, "TURN LEFT");
    else text_layer_set_text(s_dir_text_layer, "TURN AROUND");
  }
  layer_mark_dirty(s_arrow_layer);
}

static void up_click_handler(ClickRecognizerRef r, void *c) {
  DictionaryIterator *i; app_message_outbox_begin(&i);
  dict_write_int(i, MESSAGE_KEY_ID_BUTTON, &(int){1}, 4, true); app_message_outbox_send();
}
static void select_click_handler(ClickRecognizerRef r, void *c) { s_inverted = !s_inverted; update_ui_colors(); }
static void down_click_handler(ClickRecognizerRef r, void *c) {
  DictionaryIterator *i; app_message_outbox_begin(&i);
  dict_write_int(i, MESSAGE_KEY_ID_BUTTON, &(int){2}, 4, true); app_message_outbox_send();
}

static void click_config_provider(void *c) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  GRect b = layer_get_bounds(window_get_root_layer(window)); 
  s_sw = PBL_IF_ROUND_ELSE(0, 15); 

  s_arrow_layer = layer_create(GRect(0, 0, b.size.w - s_sw, b.size.h));
  layer_set_update_proc(s_arrow_layer, arrow_update_proc);
  layer_add_child(window_get_root_layer(window), s_arrow_layer);
  s_arrow_path = gpath_create(&ARROW_POINTS);
  
  s_sidebar_layer = layer_create(GRect(b.size.w - s_sw, 0, s_sw, b.size.h));
  layer_set_update_proc(s_sidebar_layer, sidebar_update_proc);
  layer_add_child(window_get_root_layer(window), s_sidebar_layer);

  s_dist_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(20, 10), PBL_IF_ROUND_ELSE(25, 5), b.size.w - s_sw - PBL_IF_ROUND_ELSE(40, 15), 45));
  text_layer_set_background_color(s_dist_layer, GColorClear);
  text_layer_set_font(s_dist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_dist_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_dist_layer));

  s_hint_layer = text_layer_create(GRect(10, b.size.h / 2 - 35, b.size.w - s_sw - 20, 70));
  text_layer_set_text(s_hint_layer, "Open Settings\nor press P\nto Pin Position");
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_hint_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hint_layer));

  s_dir_text_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(20, 10), b.size.h - PBL_IF_ROUND_ELSE(75, 55), b.size.w - s_sw - PBL_IF_ROUND_ELSE(40, 15), 30));
  text_layer_set_background_color(s_dir_text_layer, GColorClear);
  text_layer_set_font(s_dir_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_dir_text_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_dir_text_layer));

  s_time_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(30, 10), b.size.h - PBL_IF_ROUND_ELSE(45, 25), 50, 20));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  s_acc_layer = text_layer_create(GRect(10, b.size.h - PBL_IF_ROUND_ELSE(45, 25), b.size.w - s_sw - 20, 20));
  text_layer_set_background_color(s_acc_layer, GColorClear);
  text_layer_set_font(s_acc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_acc_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_acc_layer));

  s_battery_layer = text_layer_create(GRect(b.size.w - s_sw - PBL_IF_ROUND_ELSE(80, 50), b.size.h - PBL_IF_ROUND_ELSE(45, 25), 40, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));

  if (s_sw > 0) {
    s_label_s = text_layer_create(GRect(b.size.w - s_sw, 25, s_sw, 30));
    s_label_i = text_layer_create(GRect(b.size.w - s_sw, b.size.h / 2 - 15, s_sw, 30));
    s_label_p = text_layer_create(GRect(b.size.w - s_sw, b.size.h - 55, s_sw, 30));
    TextLayer *labels[] = {s_label_s, s_label_i, s_label_p}; char *texts[] = {"S", "I", "P"};
    for(int i=0; i<3; i++) {
      text_layer_set_text(labels[i], texts[i]); text_layer_set_text_alignment(labels[i], GTextAlignmentCenter);
      text_layer_set_font(labels[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      layer_add_child(window_get_root_layer(window), text_layer_get_layer(labels[i]));
    }
  }
  
  update_ui_colors(); app_message_register_inbox_received(in_received_handler);
  app_message_open(512, 64); compass_service_subscribe(compass_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)update_time);
  battery_state_service_subscribe(handle_battery); handle_battery(battery_state_service_peek()); update_time();
}

static void main_window_unload(Window *window) {
  gpath_destroy(s_arrow_path); compass_service_unsubscribe(); tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  text_layer_destroy(s_dist_layer); text_layer_destroy(s_time_layer); text_layer_destroy(s_battery_layer); 
  text_layer_destroy(s_dir_text_layer); text_layer_destroy(s_hint_layer); text_layer_destroy(s_acc_layer);
  if (s_sw > 0) { text_layer_destroy(s_label_s); text_layer_destroy(s_label_i); text_layer_destroy(s_label_p); }
  layer_destroy(s_arrow_layer); layer_destroy(s_sidebar_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) { .load = main_window_load, .unload = main_window_unload });
  window_stack_push(s_main_window, true);
}

static void deinit() { window_destroy(s_main_window); }
int main() { init(); app_event_loop(); deinit(); return 0; }