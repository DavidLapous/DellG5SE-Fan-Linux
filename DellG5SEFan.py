#! /usr/bin/python

import sys
import os
import time
import argparse
import configparser

# Get home path, config path. hwmon path
user=os.getlogin() # There may exists some cases where it's not working ?  
home = str(os.path.expanduser('~'+user))
configPath=home+"/.DellG5SE-fans.conf"
hwmon= '/sys/class/hwmon/'


# Read arguments. Everything is optional, if not mentionned, value gets retrieved in config file.
parser = argparse.ArgumentParser(description="Controls and monitor Dell G5 SE laptop fans.")
parser.add_argument("--profile", "-p", type=str, help="Use a saved profile.", default="Default")
parser.add_argument("-temp", type=float, nargs=2, metavar=("low","high"),  help="Temperature (in °C) at which fans starts spinning and at which fans are set to full speed.")
parser.add_argument("-timer", type=float, help="Time for each temperature check and fan update (in seconds).")
parser.add_argument("-s","--silent", help="Silent output.", action="store_true")
parser.add_argument("--save", help="Save profile to config file.", action="store_true")
parser.add_argument("-set", type=int, nargs=2, metavar=("low","high"), help="Set fans speed to selected value (int from 0 to 255).")

args = parser.parse_args()




# Disables prints if silent is true
if args.silent:
    sys.stdout = open(os.devnull, 'w')



# Default config. Overwritten by config file and arguments
config = configparser.ConfigParser()
config["Default"] = {"lowtemp" : "50", "hightemp" : "65", "timer" : "5"}

# Check for config file, and creating a default one if none exists.
try:
    open(configPath).read()
except IOError as e:
    print("No default config file found. Creating one at : "+configPath)
    config.write(open(configPath,"w"))
    

# Retrieve config file
config.read(configPath)


## Read config file by profile
profile = args.profile # = Default if not mentionned in args
# Check if profile is complete
try:
    if args.temp ==None:
        config[profile].getfloat("lowtemp")
        config[profile].getfloat("hightemp")
    if args.timer==None:
        config[profile].getfloat("timer")
except KeyError as e:
    print(e, "missing.")
    print("Your profile " + profile+ " is not complete, give more arguments or complete it.")
    sys.exit()

if args.temp == None:
    lowtemp =  config[profile].getfloat("lowtemp")
    hightemp = config[profile].getfloat("hightemp")
else:
    lowtemp, hightemp = args.temp
if args.timer == None:
    timer = config[profile].getfloat("timer")
else:
    timer = args.timer

# Save profile if asked
if args.save:
    config[profile]={"lowtemp" : lowtemp, "hightemp" : hightemp, "timer" : timer}
    config.write(open(configPath,"w"))


# retrieve set arguments if they exist.
set = False
if args.set != None:
    set = True
    setCPUFan, setGPUFan = args.set
    setCPUFan = min(max(setCPUFan,0),255)
    setGPUFan = min(max(setGPUFan,0),255)
    

######################################################################################



# Get dell smm and k10temp directories in hwmon
cname=""
for _,dirs,_  in os.walk(hwmon):
    for dir in dirs:
        if open(hwmon + dir + '/name').read() == "dell_smm\n":
            dell_smm = hwmon + dir
        if open(hwmon + dir + '/name').read() == "k10temp\n":
            cput = hwmon + dir
            cname="k10temp"
        if open(hwmon + dir + '/name').read() == "zenpower\n":
            cput = hwmon + dir
            cname="zenpower"


# Fan write permission check.
try:
    open(dell_smm+"/pwm1",'w').write("128")
except IOError as e:
    print(e)
    sys.exit("Cannot change fan speed. Are you running the script with root permission ?")

# If set is true, put fans to selected values cfan & gfan.
if set:
    open(dell_smm+"/pwm1",'w').write(str(setCPUFan))
    open(dell_smm+"/pwm3",'w').write(str(setGPUFan))
    print("Set fans to",setCPUFan,"and",setGPUFan)
    sys.exit()


# Print values (for debug mostly)

print("Temperatures thresholds :", lowtemp,"°C and",hightemp,"°C")
print("Loop timer :", timer)
print("Profile used :",profile)
print("Hwmon devices : dell smm is",dell_smm[-1], "and",cname, "is",cput[-1])

print("------------------------------------------------")
# Fan loop
while True:
    # # need to be placed here or it does not work -- need investigation
    # cpu_temp = open(cput+"/temp2_input",'r')
    # gpu_temp = open(dell_smm+"/temp4_input",'r')

    ctemp = float(open(cput+"/temp2_input",'r').read())/1000
    gtemp = float(open(dell_smm+"/temp4_input",'r').read())/1000

    cfan = int(open(dell_smm+"/fan1_input").read())
    gfan = int(open(dell_smm+"/fan3_input").read())

    # print update
    print("current fan speeds :", round(cfan,1),"RPM and",round(gfan,1), "RPM             ")
    print("cpu and gpu temps :", round(ctemp,1) ,"°C and ",round(gtemp,1),"°C             ")
    sys.stdout.write("\033[2F") # move the cursor 2 lines above.

    # Handles cpu fan
    if ctemp < lowtemp :
        if cfan > 2500:
            open(dell_smm+"/pwm1",'w').write("0")

    elif ctemp < hightemp:
        if not(2000 < cfan < 3500): # BIOS can set fans to only 2400 RPM which is more silent than the lowest (nonzero) dell-smm can (3000 RPM).
            open(dell_smm+"/pwm1",'w').write("128")

    else: # ctemp is high here
        if cfan < 4000:
            open(dell_smm+"/pwm1",'w').write("255")

    # Handles gpu fan
    if gtemp < lowtemp :
        if gfan > 2500:
            open(dell_smm+"/pwm3",'w').write("0")

    elif gtemp < hightemp:
        if not(2000 < gfan < 3500):
            open(dell_smm+"/pwm3",'w').write("128")

    else: # gtemp is high here
        if gfan < 4000:
            open(dell_smm+"/pwm3",'w').write("255")

    time.sleep(timer)
