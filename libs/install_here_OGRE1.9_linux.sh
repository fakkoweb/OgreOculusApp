# INSTALL DEPENDENCIES BEFORE RUNNING THIS SCRIPT (either locally or system)!!
echo "Cloning Ogre1.9 from source repo..."
hg clone https://bitbucket.org/sinbad/ogre ./ogre_repo
echo "Removing currently installed Ogre1.9 (if any)..."
rm -R OGRE
mkdir OGRE
echo "Moving Ogre Dependencies to Ogre source folder ..."
mv ./OgreDeps ./ogre_repo/Dependencies
echo "Building Ogre1.9 ..."
cd ogre_repo
hg pull && hg update v1-9-0
mkdir build
cmake . -Bbuild -DOGRE_USE_BOOST=0 -DCMAKE_INSTALL_PREFIX:PATH=../../OGRE/
cd build
make -j3
echo "Installing Ogre1.9 locally..."
make install
cd .. && cd ..
echo "Removing source files..."
echo "ogre_repo contains the source files of Ogre1.9. Remove it if you don't need it anymore."
echo "Ogre1.9 libraries successfully downloaded, compiled and installed in 'Ogre' directory."
