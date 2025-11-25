## Wheel Control
Coarse wheel control is done on topic `/<wheel_name>/cmd_vel`
- Message type: `Float32`
- Interpretation: desired wheel output in radians/s, where positive values move the rover forward

Fine wheel control is done by action server `/<wheel_name>/fine_control`
- Action type: `motor_control/action/FineWheelControl`
    - Goal fields:
        - `target_position` (float64): desired wheel position offset in radians (relative to current position)
        - `max_velocity` (float64): maximum wheel velocity in radians/s
        - `tolerance` (float64): acceptable error in radians
        - `timeout` (float32): if the action takes longer than this number of seconds, it will abort (set to negative for no timeout)
    - Result fields:
        - `final_position` (float64): final wheel position in radians (relative to starting position)
        - `position_error` (float64): final error in radians
    - Feedback fields:
        - `current_position` (float64): current wheel position in radians (relative to starting position)
        - `position_error` (float64): current error in radians
        - `percent_complete` (float64): estimated percentage of completion
        - `time_elapsed` (float32): time elapsed since start of action in seconds
- **When an action is active, coarse control commands are ignored.**

### Diagnostic Topics:
- `/<wheel_name>/current_mode`
    - Message type: `Int8`
    - Interpretation: current wheel control mode
        - `0`: velocity control mode
        - `1`: position control mode
    - Defined in `motor_control/include/motor_control/WheelMotorControlMode.hpp`
- `/<wheel_name>/current_vel`
    - Message type: `Float32`
    - Interpretation: current wheel velocity in radians/s
- `/<wheel_name>/current_position`
    - Message type: `Float64`
    - Interpretation: current wheel position in radians relative to starting position, this will gain inaccuracy over time, but this is negligible for our use case