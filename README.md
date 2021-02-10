# DellG5SE-Fan-Linux
The Dell G5SE-5505 laptop isn't working with usual fan managers, this script is a small utility to automatically set fan speed according to cpu and gpu thermals.
## Requirements
This is a python script, you'll need python, and the `sys`, `os`, `time` python libraries.
You will also need to modprobe the dell smm kernel module, which is not loaded by default on this laptop.
```shell
$ sudo modprobe dell-smm-hwmon restricted=0 ignore_dmi=1
```
If you want this setting to stay upon reboot, you can create a / replace by or append to the config file  `/etc/modules-load.d/dell-smm-hwmon.conf` 
```shell
dell-smm-hwmon
```
and the same for `/etc/modprobe.d/dell-smm-hwmon.conf` 
```shell
# This file must be at /etc/modprobe.d/
options dell-smm-hwmon restricted=0 ignore_dmi=1
```
## Usage
```shell
$ python DellG5SEFan.py -h
usage: DellG5SEFan.py [-h] [--profile PROFILE] [-temp low high] [-timer TIMER]
                      [-s] [--save]

Controls and monitor Dell G5 SE laptop fans.

optional arguments:
  -h, --help         show this help message and exit
  --profile PROFILE  Use a saved profile.
  -temp low high     Temperature (in °C) at which fans starts spinning and at
                     which fans are set to full speed.
  -timer TIMER       Time for each temperature check and fan update (in
                     seconds).
  -s, --silent       Silent output.
  --save             Save profile to config file.

```
For instance, 
```shell
$ sudo python DellG5SEFan.py -temp 50 65 -timer 2
```
will disable fans for temperature below 50°C, put fans at 50% between 50°C and 65°C and at 100% over 65°C, with a 2 second loop.

and

```shell
$ sudo python DellG5SEFan.py
```
will start the script with the `Default` profile (or create it, if it doesn't exist).
:warning: Be careful ! Too high temperatures can permanently damage your hardware.

## Todo
- [x] Make a config file
  - [ ] Live update on temp changes in config file
- [ ] Core 
  - [ ] Different temp threshold for enabling disabling to avoid repeated fan switches, or put a sleep timer after a fan change.
  - [x] Fan profiles
  - [x] change argument parsing
    - [x] non verbose mode
    - [x] default setup
    - [x] help menu
- [ ] Depending on if the script takes too much hardware resources, switch to another language.
- [ ] Once the config file is done and the script can live-update the parameters, add a gnome extension to update config file.
- [ ] Tell me 
