cd OculusSDK
git reset --hard HEAD
git checkout tags/0.5
./ConfigureDebian.sh
make release
echo "Oculus standard 0.5 libraries loaded in 'OculusSDK' and compiled. Run 'ovrd' service from 'Service' folder before plugging the Rift."
