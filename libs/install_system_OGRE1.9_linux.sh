# INSTALL DEPENDENCIES FIRST!
hg clone https://bitbucket.org/sinbad/ogre ./ogrerepo
cd ogrerepo
hg pull && hg update v1-9-0
mkdir build
cmake . -Bbuild -DOGRE_USE_BOOST=0
cd build
make -j3
make install
cd .. && cd ..
rm -R ogrerepo
echo "Ogre libraries successfully downloaded, compiled and installed in system."
