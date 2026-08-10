#include "drill/volatile_state.h"

volatile state_t g_machine_state;

void machine_init(void)
{
  g_machine_state = STATE_IDLE;
}

void machine_start(void)
{
  g_machine_state = STATE_RUNNING;
}

void machine_stop(void)
{
  g_machine_state = STATE_STOPPED;
}

state_t machine_get_state(void)
{
  return g_machine_state;
}

int machine_is_idle(void)
{
  return g_machine_state == STATE_IDLE ? 1 : 0;
}

int machine_is_running(void)
{
  return g_machine_state == STATE_RUNNING ? 1 : 0;
}

int machine_is_stopped(void)
{
  return g_machine_state == STATE_STOPPED ? 1 : 0;
}

void machine_simulate_external_change(state_t new_state)
{
  g_machine_state = new_state;
}
