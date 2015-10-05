export fn=./cmake_install.sh
wget -O $fn http://www.cmake.org/files/v2.8/cmake-2.8.12-Linux-i386.sh 1>&2
echo "First press Enter to continue. Then press Q and then say yes to all." && read
bash ${fn}
echo "cmake2.8.12 is now locally installed in current directory."
echo "Creating soft-links in main project folder..."
ln -f -s ./cmake-*/bin/cmake mycmake
ln -f -s ./cmake-*/bin/ccmake myccmake
echo "Creating soft-links in libs folder..."
cd libs
ln -f -s ./cmake-*/bin/cmake mycmake
ln -f -s ./cmake-*/bin/ccmake myccmake
cd ..
echo "Two soft-links have been created in current and libsfolder.\nCopy them with 'cp --preserve=links' to use this version of cmake from other folders.\nRun './mycmake' instead of 'cmake'.\nRun './myccmake' instead of 'ccmake'."
rm -R ./cmake_install.sh
