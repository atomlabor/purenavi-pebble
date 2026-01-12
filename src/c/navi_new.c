#include <pebble.h>

#define ID_DIST 0
#define ID_BUTTON 3
#define ID_VIBE 4
#define ID_BEARING 5
#define CONFIG_LANG 13

static Window *s_main_window;
static Layer *s_arrow_layer;
static TextLayer *s_dist_layer;
static TextLayer *s_dir_text_layer;
static TextLayer *s_label_s, *s_label_i, *s_label_l;
static Layer *s_sidebar_layer;

static char s_buffer[64];
static int s_bearing = 0;
static bool s_inverted = false;
static int s_lang = 0; 
static bool s_gps_fixed = false;
static AppTimer *s_gps_timeout_timer = NULL;

static GPath *s_arrow_path = NULL;

static const GPathInfo ARROW_POINTS = {
  .num_points = 4,
  .points = (GPoint []) {
    {0, -38}, {18, 28}, {0, 10}, {-18, 28} 
  }
};

static void gps_timeout_handler(void *data) {
  s_gps_fixed = false;
  layer_mark_dirty(s_arrow_layer);
  s_gps_timeout_timer = NULL;
}

static void sidebar_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor bar_bg = s_inverted ? GColorBlack : GColorWhite;
  GColor bar_fg = s_inverted ? GColorWhite : GColorBlack;

  graphics_context_set_fill_color(ctx, bar_bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Vertikale Trennlinie zum Hauptfeld
  graphics_context_set_stroke_color(ctx, bar_fg);
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(0, bounds.size.h));

  // Horizontale Trenner zwischen den Buttons
  graphics_draw_line(ctx, GPoint(0, 56), GPoint(bounds.size.w, 56));
  graphics_draw_line(ctx, GPoint(0, 112), GPoint(bounds.size.w, 112));
}

static void arrow_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2 + 3);
  
  GColor bg_color = s_inverted ? GColorWhite : GColorBlack;
  GColor fg_color = s_inverted ? GColorBlack : GColorWhite;

  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Horizontale Trennlinie zum unteren Textbalken
  graphics_context_set_stroke_color(ctx, fg_color);
  graphics_draw_line(ctx, GPoint(0, 137), GPoint(bounds.size.w, 137));

  if (!s_gps_fixed) {
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, center, 18);
    graphics_draw_line(ctx, GPoint(center.x - 10, center.y - 10), GPoint(center.x + 10, center.y + 10));
    graphics_draw_line(ctx, GPoint(center.x + 10, center.y - 10), GPoint(center.x - 10, center.y + 10));
  } else {
    graphics_context_set_fill_color(ctx, fg_color);
    gpath_rotate_to(s_arrow_path, (TRIG_MAX_ANGLE * s_bearing) / 360);
    gpath_move_to(s_arrow_path, center);
    gpath_draw_filled(ctx, s_arrow_path);
  }
}

static const char* get_direction_text(int angle) {
  if (!s_gps_fixed) return (s_lang == 1) ? "WAITING..." : "SUCHE GPS...";
  if (s_lang == 1) {
    if (angle > 337 || angle <= 22) return "STRAIGHT";
    if (angle > 22 && angle <= 67) return "HALF RIGHT";
    if (angle > 67 && angle <= 112) return "RIGHT";
    if (angle > 112 && angle <= 157) return "HARD RIGHT";
    if (angle > 157 && angle <= 202) return "TURN AROUND";
    if (angle > 202 && angle <= 247) return "HARD LEFT";
    if (angle > 247 && angle <= 292) return "LEFT";
    if (angle > 292 && angle <= 337) return "HALF LEFT";
  } else {
    if (angle > 337 || angle <= 22) return "GERADE";
    if (angle > 22 && angle <= 67) return "HALB RECHTS";
    if (angle > 67 && angle <= 112) return "RECHTS";
    if (angle > 112 && angle <= 157) return "HARD RECHTS";
    if (angle > 157 && angle <= 202) return "WENDEN";
    if (angle > 202 && angle <= 247) return "HARD LINKS";
    if (angle > 247 && angle <= 292) return "LINKS";
    if (angle > 292 && angle <= 337) return "HALB LINKS";
  }
  return "- - -";
}

static void update_ui_colors() {
  GColor main_bg = s_inverted ? GColorWhite : GColorBlack;
  GColor main_fg = s_inverted ? GColorBlack : GColorWhite;
  GColor bar_bg  = s_inverted ? GColorBlack : GColorWhite;
  GColor bar_fg  = s_inverted ? GColorWhite : GColorBlack;

  window_set_background_color(s_main_window, main_bg);
  text_layer_set_text_color(s_dist_layer, main_fg);
  text_layer_set_text_color(s_dir_text_layer, bar_fg);
  text_layer_set_background_color(s_dir_text_layer, bar_bg);
  text_layer_set_text_color(s_label_s, bar_fg);
  text_layer_set_text_color(s_label_i, bar_fg);
  text_layer_set_text_color(s_label_l, bar_fg);
  
  layer_mark_dirty(s_sidebar_layer);
  layer_mark_dirty(s_arrow_layer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  if (dict_find(iter, ID_BEARING) || dict_find(iter, ID_DIST)) {
    s_gps_fixed = true;
    if (s_gps_timeout_timer) app_timer_cancel(s_gps_timeout_timer);
    s_gps_timeout_timer = app_timer_register(15000, gps_timeout_handler, NULL);
  }
  Tuple *t_lang = dict_find(iter, CONFIG_LANG);
  if (t_lang) { s_lang = t_lang->value->int32; }
  Tuple *t_dist = dict_find(iter, ID_DIST);
  if (t_dist) { snprintf(s_buffer, sizeof(s_buffer), "%s", t_dist->value->cstring); text_layer_set_text(s_dist_layer, s_buffer); }
  Tuple *t_bearing = dict_find(iter, ID_BEARING);
  if (t_bearing) { s_bearing = t_bearing->value->int32; text_layer_set_text(s_dir_text_layer, get_direction_text(s_bearing)); layer_mark_dirty(s_arrow_layer); }
  if (dict_find(iter, ID_VIBE)) vibes_double_pulse();
}

static void select_click_handler(ClickRecognizerRef r, void *c) { s_inverted = !s_inverted; update_ui_colors(); }
static void up_click_handler(ClickRecognizerRef r, void *c) { 
  DictionaryIterator *iter; app_message_outbox_begin(&iter);
  int val = 1; dict_write_int(iter, ID_BUTTON, &val, sizeof(int), true); app_message_outbox_send();
}
static void down_click_handler(ClickRecognizerRef r, void *c) { light_enable_interaction(); }

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  s_arrow_layer = layer_create(GRect(0, 0, 128, 168));
  layer_set_update_proc(s_arrow_layer, arrow_update_proc);
  layer_add_child(window_layer, s_arrow_layer);
  s_arrow_path = gpath_create(&ARROW_POINTS);

  s_sidebar_layer = layer_create(GRect(128, 0, 16, 168));
  layer_set_update_proc(s_sidebar_layer, sidebar_update_proc);
  layer_add_child(window_layer, s_sidebar_layer);

  s_label_s = text_layer_create(GRect(128, 18, 16, 20));
  text_layer_set_text(s_label_s, "S");
  text_layer_set_font(s_label_s, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_label_s, GTextAlignmentCenter);
  text_layer_set_background_color(s_label_s, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_label_s));

  s_label_i = text_layer_create(GRect(128, 74, 16, 20));
  text_layer_set_text(s_label_i, "I");
  text_layer_set_font(s_label_i, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_label_i, GTextAlignmentCenter);
  text_layer_set_background_color(s_label_i, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_label_i));

  s_label_l = text_layer_create(GRect(128, 130, 16, 20));
  text_layer_set_text(s_label_l, "L");
  text_layer_set_font(s_label_l, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_label_l, GTextAlignmentCenter);
  text_layer_set_background_color(s_label_l, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_label_l));

  s_dist_layer = text_layer_create(GRect(4, 4, 120, 45)); 
  text_layer_set_font(s_dist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_dist_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_dist_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_dist_layer));

  s_dir_text_layer = text_layer_create(GRect(0, 138, 128, 30));
  text_layer_set_font(s_dir_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_dir_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_dir_text_layer));

  update_ui_colors();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_dist_layer); text_layer_destroy(s_dir_text_layer);
  text_layer_destroy(s_label_s); text_layer_destroy(s_label_i); text_layer_destroy(s_label_l);
  gpath_destroy(s_arrow_path); layer_destroy(s_arrow_layer); layer_destroy(s_sidebar_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) { .load = main_window_load, .unload = main_window_unload });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(in_received_handler);
  app_message_open(256, 64);
}

static void deinit() { window_destroy(s_main_window); }
int main(void) { init(); app_event_loop(); deinit(); }