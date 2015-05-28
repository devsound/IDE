#include <stdint.h>
#include <stdbool.h>

typedef struct pid_t {
  // Configuration
  double             kp, ki, kd;         // pid:          control terms
  double             input;              // pid:          input    (CV)
  double             setpoint;           // pid:          setpoint (SP)
  double             output;             // pid:          output   (MV)
  volatile uint32_t *timer;              // timebase:     timer pointer
  double             timebase;           // timebase:     timer rate
  double             minimum_dt;         // timebase:     minimum time between calculations
  double             min, max;           // output-limit: min and max
  unsigned           deadband_mode;      // deadband:     mode (deadband_e)
  double             deadband_range;     // deadband:     output range
  double             deadband_time;      // deadband:     bypass interval
  unsigned           windup_mode;        // anti-windup:  mode
  double             windup_limit;       // anti-windup:  anti-windup max integral limiting
  // Internals
  double             proportional;       // pid:          proportional term
  double             integral;           // pid:          integral term
  double             derivative;         // pid:          derivative term
  uint32_t           timer_mem;          // pid:          timer for delta time (dt)
  uint32_t           deadband_timer_mem; // deadband:     timer for bypass time (bypass_dt)
  bool               windup_stop;        // anti-windup:  anti-windup enabled flag
} pid_t;

// Deadband modes
enum deadband_e {
  DEADBAND_DISABLED   = 0, // Do not use deadband management
  DEADBAND_CLASSIC,        // Classical anti-windup (requires minimum change)
  DEADBAND_HYSTERESIS,     // Hysteresis anti-windup (delays on direction change)
};

// Anti-windup modes
enum windup_e {
  ANTIWINDUP_DISABLED = 0, // Do not use anti-windup management
  ANTIWINDUP_ENABLED,      // Disabled (integral is blocked)
  ANTIWINDUP_MAX,          // Enabled when above set threshold
  ANTIWINDUP_ON_LIMIT,     // Enabled when output limits are hit
};

// Set defaults
pid_begin(pid_t *ctx) {
  ctx->timebase = 1000;
  ctx->timer_mem = millis();
}

// Configure timebase
pid_timebase(pid_t *ctx, volatile uint32_t *timer, double timebase, double minimum_dt) {
  ctx->timer = timer;
  ctx->timebase = timebase;
  ctx->minimum_dt = minimum_dt;
  if(timer) ctx->timer_mem = *timer;
  else ctx->timer_mem = millis();
}

// Configure control terms
void pid_tune(pid_t *ctx, double kp, double ki, double kd) {
  ctx->kp = kp;
  ctx->ki = ki;
  ctx->kd = kd;
}

// Configure output limit
void pid_limit(pid_t *ctx, double min, double max) {
  ctx->min = min;
  ctx->max = max;
}

// Configure anti-windup
void pid_antiwindup(pid_t *ctx, unsigned mode, double limit) {
  ctx->windup_mode = mode;
  ctx->windup_limit = limit;
}

// Configure deadband
void pid_deadband(pid_t *ctx, unsigned mode, double range, double time) {
  ctx->deadband_mode = mode;
  ctx->deadband_range = range;
  ctx->deadband_time = time;
}

// Workhorse
void pid_compute(pid_t *ctx) {
  
  // Timebase management
  uint32_t timer;
  if(ctx->timer) timer = *ctx->timer;
  else timer = millis();
  uint32_t dt = (double)(timer - ctx->timer_mem) / ctx->timebase;
  if(!(dt > ctx->minimum_dt)) return; // Custom/lowest precision limit
  ctx->timer_mem = timer;

  // Main PID calculation
  double error = ctx->setpoint - ctx->input;
  if(!ctx->windup_stop) ctx->integral += error * dt;
  ctx->derivative = (error - ctx->proportional) / dt;
  ctx->proportional = error;
  double output = ctx->kp * error + ctx->ki * ctx->integral + ctx->kd * ctx->derivative;

  // Limit management
  bool limited = false;
  if(ctx->min != ctx->max) {
    if((limited = output < ctx->min)) output = ctx->min;
    else
    if((limited = output > ctx->max)) output = ctx->max;
  }

  // Anti-windup management
  if(ctx->windup_mode != ANTIWINDUP_DISABLED) {
    switch(ctx->windup_mode) {
      case ANTIWINDUP_ENABLED:
        ctx->windup_stop = true;
        break;
      case ANTIWINDUP_MAX: 
        ctx->windup_stop = (ctx->integral > ctx->windup_limit);
        break;
      case ANTIWINDUP_ON_LIMIT: 
        ctx->windup_stop = limited;
        break;
    }
  } else ctx->windup_stop = false;

  // Deadband management
  if(ctx->deadband_mode != DEADBAND_DISABLED) {
    double bypass_dt = (double)(timer - ctx->deadband_timer_mem) / ctx->timebase;
    double delta;
    if(ctx->deadband_time == 0 || bypass_dt < ctx->deadband_time) {
      switch(ctx->deadband_mode) {
        case DEADBAND_CLASSIC:
          delta = ctx->output > output ? ctx->output - output : output - ctx->output;
          if(delta < ctx->deadband_range) return;
          break;
        case DEADBAND_HYSTERESIS:
          if(output > ctx->output + ctx->deadband_range) output -= ctx->deadband_range;
          else
          if(output < ctx->output - ctx->deadband_range) output += ctx->deadband_range;
          else return;
          break;
      }
    }
  }
  ctx->deadband_timer_mem = timer;

  // Write output (deadband manager may abort before reaching here)
  ctx->output = output;
}