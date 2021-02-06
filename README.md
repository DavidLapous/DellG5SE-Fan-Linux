# DellG5SE-Fan-Linux
The Dell G5SE-5505 laptop isn't working with usual fan managers, this script is a small utility to automatically set fan speed according to thermals.

## usage
```shell
sudo python DellG5SEFan.py lowtemp maxtemp timer
```
For instance, 
```shell
sudo python DellG5SEFan.py 50 65 10
```
will disable fans for temperature below 50°C, put fans at 50% between 50°C and 65°C and at 100% over 65°C, with a 10 second loop.
