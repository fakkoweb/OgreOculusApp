# INSTALL OPENCV DEPENDENCIES BEFORE RUNNING THIS SCRIPT (either locally or system)!!
echo "Cloning OpenCV2.4 from source repo..."
git clone https://github.com/Itseez/opencv.git ./opencv_repo2.4
echo "Removing currently installed OpenCV2.4.9 (if any)..."
rm -R opencv
mkdir opencv
echo "Building OpenCV2.4 ..."
cd opencv_repo2.4
git fetch --all && git pull && git checkout 2.4 && git pull
mkdir build
cmake . -Bbuild -DOGRE_USE_BOOST=1 -DWITH_TBB=1 -DWITH_CUDA=1 -DCMAKE_INSTALL_PREFIX:PATH=../../opencv/
cd build
make -j3
echo "Installing OpenCV2.4 locally..."
make install
cd .. && cd ..
#echo "Removing source files..."
#rm -R opencv_repo
echo "OpenCV2.4 libraries successfully downloaded, compiled and installed in 'opencv' directory."
