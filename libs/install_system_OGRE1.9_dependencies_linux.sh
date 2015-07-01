# NOTE: all dependencies already present into system will be ignored (or asked to update but not necessary)

# minimum building requirements
sudo apt-get install build-essential automake libtool
# minimum ogre dependencies
sudo apt-get install libfreetype6-dev libfreeimage-dev libzzip-dev libxrandr-dev libxaw7-dev freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
# optional ogre dependencies to install
sudo apt-get install libois-dev 
# other dependencies available: nvidia-cg-toolkit libboost-thread-dev
echo "Ogre dependencies successfully downloaded and installed in system."
