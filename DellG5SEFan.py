import sys
import os
import time
# import sudo
args = sys.argv
lowtemp = float(args[1]) # Arguments 
hightemp = float(args[2])
timer = float(args[3])
hwmon= '/sys/class/hwmon/'

# Get dell smm and k10temp directories in hwmon
for _,dirs,_  in os.walk(hwmon):
    for dir in dirs:
        if open(hwmon + dir + '/name').read() == "dell_smm\n":
            dell_smm = hwmon + dir
        if open(hwmon + dir + '/name').read() == "k10temp\n":
            cput = hwmon + dir

print("dell smm is :"+dell_smm[-1], " and k10temp is :"+cput[-1])

# Fan loop
while True:
    # need to be placed here or it does not work -- need investigation
    cpu_temp = open(cput+"/temp2_input",'r')
    gpu_temp = open(dell_smm+"/temp4_input",'r')

    ctemp = float(cpu_temp.read())/1000
    gtemp = float(gpu_temp.read())/1000

    cfan = int(open(dell_smm+"/fan1_input").read())
    gfan = int(open(dell_smm+"/fan3_input").read())
    print("------------------------------------------")
    print("current fan speeds :", cfan,"RPM and",gfan, "RPM")
    print("cpu temp :", ctemp ,"°C ---- gpu temp :",gtemp,"°C")

    # Handles cpu fan
    if ctemp < lowtemp :
        if cfan > 0:
            open(dell_smm+"/pwm1",'w').write("0")

    elif ctemp < hightemp:
        if not(2500 < cfan < 3500):
            open(dell_smm+"/pwm1",'w').write("128")

    else: # ctemp is high here
        if cfan < 4000:
            open(dell_smm+"/pwm1",'w').write("255")

    # Handles gpu fan
    if gtemp < lowtemp :
        if gfan > 0:
            open(dell_smm+"/pwm3",'w').write("0")

    elif gtemp < hightemp:
        if not(2500 < gfan < 3500):
            open(dell_smm+"/pwm3",'w').write("128")

    else: # gtemp is high here
        if gfan < 4000:
            open(dell_smm+"/pwm3",'w').write("255")

    time.sleep(timer)
