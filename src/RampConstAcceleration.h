#ifndef RAMP_CONST_ACCELERATION_H
#define RAMP_CONST_ACCELERATION_H

#if (TICKS_PER_S == 16000000L)
#define UPM_TICKS_PER_S UPM_CONST_16E6
#define UPM_TICKS_PER_S_DIV_500 UPM_CONST_32000
#define UPM_TICKS_PER_S_DIV_SQRT_OF_2 UPM_CONST_16E6_DIV_SQRT_OF_2
#define UPM_ACCEL_FACTOR UPM_CONST_128E12
#define US_TO_TICKS(u32) (u32 * 16)
#define TICKS_TO_US(u32) (u32 / 16)
#elif (TICKS_PER_S == 21000000L)
#define UPM_TICKS_PER_S UPM_CONST_21E6
#define UPM_TICKS_PER_S_DIV_500 UPM_CONST_42000
#define UPM_TICKS_PER_S_DIV_SQRT_OF_2 UPM_CONST_21E6_DIV_SQRT_OF_2
#define UPM_ACCEL_FACTOR UPM_CONST_2205E11
#define US_TO_TICKS(u32) (u32 * 21)
#define TICKS_TO_US(u32) (u32 / 21)
#else
#define UPM_TICKS_PER_S upm_timer_freq
#define UPM_TICKS_PER_S_DIV_500 upm_timer_freq_div_500
// This overflows for approx. 1s at 40 MHz, only
#define US_TO_TICKS(u32) \
  ((uint32_t)((((uint32_t)((u32) * (TICKS_PER_S / 10000L))) / 100L)))

// This calculation needs more work
#define TICKS_TO_US(u32) \
  ((uint32_t)((((uint32_t)((u32) / (TICKS_PER_S / 1000000L))) / 1L)))
#endif


struct ramp_config_s {
  uint32_t min_travel_ticks;
  upm_float upm_inv_accel2;
  upm_float upm_sqrt_inv_accel;
  uint8_t accel_change_cnt;

  void init() {
  accel_change_cnt = 0;
  min_travel_ticks = 0;
  upm_inv_accel2 = 0;
  }
  inline int8_t checkValidConfig() {
	if (min_travel_ticks == 0) {
		return MOVE_ERR_SPEED_IS_UNDEFINED;
	}
    if (upm_inv_accel2 == 0) {
		return MOVE_ERR_ACCELERATION_IS_UNDEFINED;
	}
	return MOVE_OK;
  }
  inline void setSpeedInTicks(uint32_t min_step_ticks) {
     min_travel_ticks = min_step_ticks;
  }
  inline void setAcceleration(int32_t accel) {
#ifdef UPM_ACCEL_FACTOR
  upm_float new_upm_inv_accel2 =
      upm_divide(UPM_ACCEL_FACTOR, upm_from((uint32_t)accel));
#else
  upm_float upm_inv_accel =
      upm_divide(upm_shr(UPM_TICKS_PER_S, 1), upm_from(accel));
  upm_float upm_inv_accel2 = upm_multiply(UPM_TICKS_PER_S, upm_inv_accel);
#endif
  if (upm_inv_accel2 != new_upm_inv_accel2) {
    upm_inv_accel2 = new_upm_inv_accel2;

    // This is A = f / sqrt(2*a) = (f/sqrt(2))*rsqrt(a)
    upm_sqrt_inv_accel = upm_multiply(
        upm_rsqrt(upm_from((uint32_t)accel)), UPM_TICKS_PER_S_DIV_SQRT_OF_2);
    accel_change_cnt++;
  }
  }
};

struct ramp_ro_s {
  struct ramp_config_s config;
  int32_t target_pos;
  bool force_stop;
  bool keep_running;
  bool keep_running_count_up;
};

struct ramp_rw_s {
  volatile uint8_t ramp_state;
  // if accel_change_cnt does not match config.accel_change_cnt, then
  // performed_ramp_up_steps to be recalculated
  uint8_t accel_change_cnt;
  // the speed is linked on both ramp slopes to this variable as per
  //       s = v²/2a   =>   v = sqrt(2*a*s)
  uint32_t performed_ramp_up_steps;
  // Are the ticks stored of the last previous step, if pulse time requires
  // more than one command
  uint32_t pause_ticks_left;
  // Current ticks for ongoing step
  uint32_t curr_ticks;
  void init() {
  pause_ticks_left = 0;
  performed_ramp_up_steps = 0;
  accel_change_cnt = 0xff;
  ramp_state = RAMP_STATE_IDLE;
  curr_ticks = TICKS_FOR_STOPPED_MOTOR;
  }
};

class NextCommand {
 public:
  struct stepper_command_s command;
  struct ramp_rw_s rw;
};

void _getNextCommand(const struct ramp_ro_s *ramp,
                            const struct ramp_rw_s *rw,
                            const struct queue_end_s *queue_end,
                            NextCommand *command);
#endif

