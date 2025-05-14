const int screen_w = 1920;
const int screen_h = 1080;
const int tickrate = 60;
const int target_fps = 60;
const double max_delta_time = 0.25;

void fixed_update(double fixed_delta);
void render(void);
void render_present(void);
void resize_window(int w, int h);
void update(void);