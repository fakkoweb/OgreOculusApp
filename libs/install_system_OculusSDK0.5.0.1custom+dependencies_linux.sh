git clone https://github.com/fakkoweb/OculusSDK.git ./oculusrepo
cd oculusrepo
git checkout 0.5_extended_prediction
./ConfigureDebian.sh
make release
sudo make install
rm -R oculusrepo
echo "Oculus 0.5.0.1 dependencies installed in system. Rules file installed. Oculus libraries successfully downloaded, compiled and installed in system. Run 'ovrd' service from 'Service' folder before plugging the Rift."
