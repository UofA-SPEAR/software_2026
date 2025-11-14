If the host is Wayland, run `sudo xhost +si:localuser:($USERNAME)` from the host for display forwarding
- TODO: This resets on reboot, how to make it permanent?

For WSL: you may need to export $USERNAME from your bashrc:
- in wsl terminal: `echo $USERNAME`
- if it prints out a username you're happy with, you can skip the steps below
- otherwise, or if you want something different, do the following:
1. `nano ~/.bashrc`
2. navigate to the bottom and write `export USERNAME=$(whoami)`
    - you can also put `export USERNAME=your_preferred_username` if you want something different
3. save and exit (CTRL + S, CTRL + X)
4. `source ~/.bashrc`
5. `echo $USERNAME` to verify it worked and prints out what you want

