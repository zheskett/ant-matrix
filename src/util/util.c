#include "util/util.h"

int nearest_16_by_9_height(int width) {
  // Nearest 16:9 standard resolution, 1 res below
  if (width <= 640) {
    return 240;
  } else if (width <= 854) {
    return 360;
  } else if (width <= 1280) {
    return 480;
  } else if (width <= 1920) {
    return 720;
  } else if (width <= 2560) {
    return 1080;
  } else if (width <= 3840) {
    return 1440;
  } else {
    return 2160;
  }
}