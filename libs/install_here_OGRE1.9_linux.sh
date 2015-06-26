sudo apt-get install build-essential automake libtool
sudo apt-get install libfreetype6-dev nvidia-cg-toolkit libfreeimage-dev zlib1g-dev libzzip-dev libois-dev libcppunit-dev doxygen libxt-dev libxaw7-dev libxxf86vm-dev libxrandr-dev libglu-dev
hg clone https://bitbucket.org/sinbad/ogre ./ogrerepo
rm -R OGRE
mkdir OGRE
cd ogrerepo
hg pull && hg update v1-9-0
mkdir build
cmake . -Bbuild -DOGRE_USE_BOOST=0 -DCMAKE_INSTALL_PREFIX:PATH=../../OGRE/
cd build
make -j3
make install
cd .. && cd ..
rm -R ogrerepo
echo "Ogre libraries successfully downloaded, compiled and installed in 'OGRE' directory."
