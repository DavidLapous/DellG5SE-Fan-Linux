# DellG5SE-Fan-Linux
The Dell G5SE-5505 laptop isn't working with usual fan managers, this script is a small utility to automatically set fan speed according to cpu and gpu thermals.
## Requirements
You will need to modprobe the dell smm kernel module, which is not loaded by default on this laptop. For the CPP version, which is writing to directly to the embedded controller of the laptop, you will need to allow EC writing.

**Be aware that this binary may hurt your hardware if you don't have the exact same hardware as mine (EC chip). Correct the EC offsets in the sources files according to your hardware if necessary.**

```shell
$ sudo modprobe dell-smm-hwmon restricted=0 ignore_dmi=1
$ sudo modprobe ec_sys write_support=1 
```
If you want these settings to stay upon reboot, you can create a / replace by or append to the config file  `/etc/modules-load.d/dell-smm-hwmon.conf` 
```shell
# This file must be at /etc/modules-load.d/
dell-smm-hwmon
ec_sys
```
and the same for `/etc/modprobe.d/dell-smm-hwmon.conf` 
```shell
# This file must be at /etc/modprobe.d/
options dell-smm-hwmon restricted=0 ignore_dmi=1
options ec_sys write_support=1
```
If `ec_sys` still doesn't load, it may be because it's a builtin kernel module. Then, try to add `ec_sys.write_support=1` as a kernel paramenter. 

## Usage

### Cpp Version
Approximatively the same as the python script as usage. EC writing gives (much more) fan control.
```shell
$ DellFan -h
Usage :
sudo DellFan [-s left_fan_speed right_fan_speed] [-l t1 t2 t3 t4] [-t timer] [-r ] [-b]

Arguments description :
 -h, --help             show this help message and exit.
 -s, --set left right   sets left/rights fan to selected speed (from 0 to 255).
 -l, --loop t1 t2 t3 t4 given the temperature thresholds t1, t2, t3, t4 (in °C),
                        adjusts the fan speed accordingly with a loop.
 -t, --timer t          set the loop timer (in seconds). Default is 20s.
 -m, --manual           Switch to manual fan mode.
 -r, --restore          Gives back the fan control to the BIOS.
 -b, --boost            Set fan speed to BOOST (as fn+F7).
 -v, --verbose          Prints loop fan updates. -1 means no fan update made.

```
![Fan curve](https://raw.githubusercontent.com/DavidLapous/DellG5SE-Fan-Linux/main/fan_curve.svg)

For a better ease of use, you can put this script in your binaries path `/usr/bin/DellFan` and ask `sudo` not to ask your password everytime you use it by adding the line
```
ALL ALL=(root) NOPASSWD: /usr/bin/DellFan
```
to your `sudoers` file.

This also should allow you to map profiles / fan speeds to keyboard shortcuts via your display environment (gnome / kde / ...).

:warning: Be careful ! Too high temperatures can permanently damage your hardware.

## Todo
- [x] Make a config file
  - [ ] Live update on temp changes in config file
- [ ] Gnome extension / PySimpleGUI with system tray 
  - [ ] Temperature monitoring, temp graphs.
  - [ ] Update profile / Fan speed.
  - [ ] Keyboard shortcut, complicated with wayland I don't really know how to deal with it.
- [x] Cpp version 
  - [ ] gui.
  - [ ] Remove the Dell-smm-hwmon kernel module dependance.
- [x] Direct EC editing.
  - [x] GPU part.
  - [x] CPU part.
  - [x] Manual trigger. 
- [ ] Tell me 
