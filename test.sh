echo "Building..."
colcon build --symlink-install
echo "Testing..."
colcon test
echo "Test results..."
colcon test-result --verbose