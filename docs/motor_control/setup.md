## Setting up EtherCAT on the *jetson*
Followed https://icube-robotics.github.io/ethercat_driver_ros2/quickstart/installation.html under "installing EtherLab" section
- cloned from `/tmp`, so repo is in `/tmp/ethercat`
- modifications: vim instead of gedit, on the line `MASTER0_DEVICE="ff..."`, replaced with mac address of `eth0` found by `ifconfig`

Then `sudo /etc/init.d/ethercat start`

Also, don't forget to clone the ros2-space repo in src by running `./clone_additional_repo.sh`