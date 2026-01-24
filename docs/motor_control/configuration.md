We are using `ros2_control` and `ethercat_driver_ros2` as abstraction layers for control.

## ros2_control Configuration
Ros2_control is configured in the `controllers.yaml` file, 

example:
```yaml
controller_manager:
  ros__parameters:
    update_rate: 100
    
    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster
    
    motor_controller:
      type: forward_command_controller/ForwardCommandController

joint_state_broadcaster:
  ros__parameters:
    joints:
      - motor_joint
    map_interface_to_joint_state:
      position: Feedback

motor_controller:
  ros__parameters:
    joints:
      - motor_joint
    interface_name: Target_PWM
```

You must declare the manager, and then configure the broadcaster/controller/other components you declare inside the manager's configuration


## EtherCAT configuration
Joints for ros2_control must be declared in the urdf
```xml
    <joint name="motor_joint">
      <command_interface name="Target_PWM"/>
      <state_interface name="Feedback"/>
      <ec_module name="Motor1">
        <plugin>ethercat_generic_plugins/GenericEcSlave</plugin>
        <param name="alias">0</param>
        <param name="position">0</param>
        <param name="slave_config">/home/spearua/alec/software_2026/install/motor_test/share/motor_test/config/motor_slave_config.yaml</param>
      </ec_module>
    </joint>
```
the `<ec_module>` element declares the top-level ethercat parameters (like address), but deeper configuration (pdo mappings) must be done in a configuration yaml file (in this case `motor_slave_config.yaml`). `ethercat_driver_ros2` seems to contain some existing configurations for commercial devices, since we are writing our own ethercat firmware(?), we will need to make one custom config yml per firmware type (ie per motor type)

example config.yml:
```yaml
vendor_id: 0x00001337
product_id: 0x000004d2

sm:
  - {index: 0, type: output, pdo: null}
  - {index: 1, type: input,  pdo: null}
  - {index: 2, type: output, pdo: rpdo, watchdog: true}
  - {index: 3, type: input,  pdo: tpdo, watchdog: false}

rpdo: 
  - index: 0x1600
    channels:
      - {index: 0x7000, sub_index: 1, type: uint32, command_interface: Target_PWM}

tpdo: 
  - index: 0x1a00
    channels:
      - {index: 0x6000, sub_index: 1, type: uint32, state_interface: Feedback}
```

much of this information can be gathered by running `ethercat pdos` and looking at the output, translating it to yml form

Ideally, we need 3 ymls (drive motors, steer motors, arm motors), that can be referenced multiple times each in the urdf
