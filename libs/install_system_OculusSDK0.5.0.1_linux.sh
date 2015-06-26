git clone https://github.com/fakkoweb/OculusSDK.git ./oculusrepo
cd oculusrepo
git checkout tags/0.5
./ConfigureDebian.sh
make release
sudo make install
rm -R oculusrepo
echo "Oculus libraries successfully downloaded, compiled and installed in system. Run 'ovrd' service from 'Service' folder before plugging the Rift."
