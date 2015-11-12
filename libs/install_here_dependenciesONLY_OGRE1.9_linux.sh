# DOWNLOAD COMPILE AND INSTALL LOCALLY DEPENDENCIES FIRST!
echo "Cloning Ogre Dependencies from source repo..."
hg clone https://bitbucket.org/cabalistic/ogredeps ./ogredeps_repo
echo "Removing remnants of last installed dependencies (if any)..."
rm -R OgreDeps
mkdir OgreDeps
echo "Building Ogre dependencies..."
cd ogredeps_repo
hg pull && hg update default
mkdir build
cmake . -Bbuild -DCMAKE_INSTALL_PREFIX:PATH=../../OgreDeps/
cd build
make -j3
echo "Installing Ogre dependencies locally..."
make install
cd .. & cd ..
echo "ogredeps_repo contains the source files of dependencies. Remove it if you don't need it anymore."
echo "Ogre dependencies successfully downloaded, compiled and installed in 'OgreDeps' directory."

