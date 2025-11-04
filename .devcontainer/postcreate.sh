#!/bin/sh
# set git to checkout as-is, commit LF line endings
git config --global core.autocrlf input

# Original post-create below
#sudo rosdep update && sudo rosdep install --from-paths src --ignore-src -y && sudo chown -R $(whoami) /home/ws/

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo -e "\e[3m\e[1;93mRemember to git config user.name and user.email if this is a new environment.\e[0m (don't use --global flag as it gets reset when the container is rebuilt)"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!"
