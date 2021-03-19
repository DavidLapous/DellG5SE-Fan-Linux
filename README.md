# DellG5SE-Fan-Linux
The Dell G5SE-5505 laptop isn't working with usual fan managers, this script is a small utility to automatically set fan speed according to cpu and gpu thermals.
## Requirements
Python (tested on latest version 3.9). Make sure either k10temp or zenpower (zenmonitor) kernel modules are loaded (by default k10temp is loaded). 
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

### Python version
```shell
$ python DellG5SEFan.py -h 
usage: DellG5SEFan.py [-h] [--profile PROFILE] [-temp low high] [-timer TIMER]
                      [-s] [--save] [-set cpu_fan gpu_fan]

Controls and monitor Dell G5 SE laptop fans.

optional arguments:
  -h, --help            show this help message and exit
  --profile PROFILE, -p PROFILE
                        Use a saved profile.
  -temp low high        Temperature (in °C) at which fans starts spinning and
                        at which fans are set to full speed.
  -timer TIMER          Sleep time between each temperature check and fan
                        update (in seconds).
  -s, --silent          Silent output.
  --save                Save profile to config file.
  -set cpu_fan gpu_fan  Set fans speed to selected value (integer from 0 to
                        255).


```
For instance, 
```shell
$ sudo python DellG5SEFan.py -temp 50 65 -timer 2
```
will disable fans for temperature below 50°C, put fans at 50% between 50°C and 65°C and at 100% over 65°C, with a 2 second loop,

```shell
$ sudo python DellG5SEFan.py -temp 0 65 -timer 10 -p m --save
```
will set fans to 50% and to 100% if cpu or gpu temperature is greater than 65°C, and save it to the (new) profile m,

```shell
$ sudo python DellG5SEFan.py -temp 0 0 -timer 10 -p f --save
```
will set fans to maximum speed, and save it to the (new) profile f,
```shell
$ sudo python DellG5SEFan.py
```
will start the script with the `Default` profile (or create it, if it doesn't exist).
```shell
$ sudo python DellG5SEFan.py -p m
```
will start the script with the `m` profile (if it exists).

:warning: Be careful ! Too high temperatures can permanently damage your hardware.

### Cpp Version
As the early version of the python script, 
```shell
$ sudo DellFan lowtemp hightemp timer
```
If you only want to set fan speeds once, the usage is
```shell
$ sudo DellFan cpu_fan_speed gpu_fan_speed -1
```
The fan speeds should belong to the interval [0;256].

For a better ease of use, you can put this script in your binaries path `/usr/bin/DellFan` and ask `sudo` not to ask your password everytime you use it by adding the line
```
ALL ALL=(root) NOPASSWD: /usr/bin/DellFan
```
to your `sudoers` file.

:warning: Be careful ! Too high temperatures can permanently damage your hardware.

## Todo
- [x] Make a config file
  - [ ] Live update on temp changes in config file
- [ ] Gnome extension / PySimpleGUI with system tray 
  - [ ] Temperature monitoring, temp graphs.
  - [ ] Update profile / Fan speed.
  - [ ] Keyboard shortcut, complicated with wayland I don't really know how to deal with it.
- [x] Cpp version
  - [ ] gui  
- [ ] Tell me 
