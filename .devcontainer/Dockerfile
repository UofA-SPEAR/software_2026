FROM ubuntu:22.04
ARG USERNAME=USERNAME
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG VERSION_NUMBER=VERSION_NUMBER

# Delete user if it exists in container (e.g Ubuntu Noble: ubuntu)
RUN if id -u $USER_UID ; then userdel `id -un $USER_UID` ; fi

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    #
    # [Optional] Add sudo support. Omit if you don't need to install software after connecting.
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME



# set the timezone
ENV DEBIAN_FRONTEND=noninteractive
RUN ln -fs /usr/share/zoneinfo/Etc/UTC /etc/localtime && \
    apt-get update && \
    apt-get install -y tzdata && \
    dpkg-reconfigure -f noninteractive tzdata


RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y python3-pip
ENV SHELL=/bin/bash


# Install ROS2 (Kilted)
RUN sudo apt update && sudo apt install curl -y
# clear the temp source if it exists
RUN rm -f /tmp/ros2-apt-source.deb || true
RUN export ROS_APT_SOURCE_VERSION=$VERSION_NUMBER \
    && curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb" \
    && sudo dpkg -i /tmp/ros2-apt-source.deb

RUN apt-get update && apt-get install ros-dev-tools -y

RUN apt-get update && apt-get upgrade -y

RUN apt-get install ros-humble-desktop -y

RUN apt-get install python3-colcon-common-extensions -y

# Wayland + XWayland + other
RUN apt-get install -y \
    libwayland-client0 \
    libwayland-cursor0 \
    libwayland-egl1 \
    libxkbcommon0 \
    qt6-wayland \
    xwayland \
    xauth \
    mesa-utils

# Gazebo Sim
RUN sudo apt-get install lsb-release gnupg
RUN sudo curl https://packages.osrfoundation.org/gazebo.gpg --output /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg
RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] https://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null
RUN sudo apt-get update
RUN sudo apt-get install ignition-fortress -y

# Gazebo Bridge
RUN sudo apt-get install ros-humble-ros-gz -y

# add sources to bashrc
RUN echo "source /opt/ros/humble/setup.bash" >> /home/$USERNAME/.bashrc
RUN echo "source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash" >> /home/$USERNAME/.bashrc
RUN echo "source /workspace/install/setup.bash || true" >> /home/$USERNAME/.bashrc
# ^ || true here in case the setup.bash hasn't been created yet

# Set the default user.
USER $USERNAME
CMD ["/bin/bash"]