cd OculusSDK
git reset --hard HEAD
git checkout 0.5_extended_prediction
./ConfigureDebian.sh
make release
echo "Oculus custom 0.5 libraries loaded in 'OculusSDK' and compiled. Run 'ovrd' service from 'Service' folder before plugging the Rift."
