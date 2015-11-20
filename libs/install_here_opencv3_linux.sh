# INSTALL OPENCV DEPENDENCIES BEFORE RUNNING THIS SCRIPT (either locally or system)!!
echo "Cloning OpenCV3 from source repo..."
git clone https://github.com/Itseez/opencv.git ./opencv_repo3
echo "Removing currently installed OpenCV3 (if any)..."
rm -R opencv
mkdir opencv
echo "Building OpenCV3 ..."
cd opencv_repo3
git fetch --all && git pull && git checkout master && git pull
mkdir build
cmake . -Bbuild -DOGRE_USE_BOOST=1 -DWITH_IPP=0 -DWITH_TBB=1 -DWITH_CUDA=1 -DCMAKE_INSTALL_PREFIX:PATH=../../opencv/
#WITH_IPP is disabled for a bug in configuring
cd build
make -j3
echo "Installing OpenCV3 locally..."
make install
cd .. && cd ..
#echo "Removing source files..."
#rm -R opencv_repo
echo "OpenCV3 libraries successfully downloaded, compiled and installed in 'opencv' directory."
