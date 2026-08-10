#include "drill/function_pointer.h"

#include <stdlib.h>

struct LedController {
  led_callback_t handlers[LED_COUNT];
};

struct LedController * led_controller_create(void)
{
  struct LedController * ctrl = malloc(sizeof(struct LedController));
  if (ctrl == NULL) {
    return NULL;
  }
  for (int i = 0; i < LED_COUNT; i++) {
    ctrl->handlers[i] = NULL;
  }
  return ctrl;
}

void led_controller_destroy(struct LedController * ctrl)
{
  free(ctrl);
}

void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler)
{
  ctrl->handlers[led_id] = handler;
}

void led_set(struct LedController * ctrl, int led_id, int state)
{
  if (ctrl->handlers[led_id] != NULL) {
    (*ctrl->handlers[led_id])(led_id, state);
  }
}

int led_handler_is_null(const struct LedController * ctrl, int led_id)
{
  return ctrl->handlers[led_id] == NULL ? 1 : 0;
}
