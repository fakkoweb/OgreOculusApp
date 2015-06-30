# INSTALL DEPENDENCIES FIRST!
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
