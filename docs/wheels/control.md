## Wheel Control
Coarse wheel control is done on topic `/<wheel_name>/cmd_vel`
- Message type: `Float32`
- Interpretation: desired wheel output in radians/s, where positive values move the rover forward

Fine wheel control is done by action server `/<wheel_name>/fine_control`
- Action type: `motor_control/action/FineWheelControl`
    - Goal fields:
        - `target_position` (float64): desired wheel position in radians
        - `max_velocity` (float64): maximum wheel velocity in radians/s
        - `tolerance` (float64): acceptable error in radians
        - `timeout` (float32): if the action takes longer than this number of seconds, it will cancel
    - Result fields:
        - `final_position` (float64): final wheel position in radians
        - `position_error` (float64): final error in radians
        - `success` (bool): whether the action succeeded
    - Feedback fields:
        - `current_position` (float64): current wheel position in radians
        - `position_error` (float64): current error in radians
        - `percent_complete` (float64): estimated percentage of completion
        - `time_elapsed` (float32): time elapsed since start of action in seconds
- **When an action is active, coarse control commands are ignored.**
