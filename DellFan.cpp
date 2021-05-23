// #include "FindHwmon.h"
#include <stdio.h>
#include <iostream>
#include <string>
// #include <cstring>
#include <algorithm>  // std::max
#include <fstream>    //read  and write files
#include <filesystem> // file browsing
#include <unistd.h>   // sleep library
// #include <stdexcept> //error handling
// #include <gtk/gtk.h> // gtk interface

// #include <getopt.h> //parsing

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

namespace fs = std::filesystem;

#define ECio "/sys/kernel/debug/ec/ec0/io"

// Byte offsets
#define ManualECMode_cpu 147 // 0x93
#define ManualECMode_gpu 150 // 0x96
#define GPUaddr 151 // 0x97
#define CPUaddr 148 //0x94

// Hex fan speeds
#define ZERO 255 // 0xFF -- 0 RPM
#define SLOW 245 // 0xF5 -- 2000RPM
#define MEDIUM 200 // 204 is 0xCC -- 2400 RPM
#define NORMAL 163 // 0xA3 -- 3000 RPM
#define FAST 102 // 0x66 -- 4800 RPM
#define BOOST 91 // 0x5B -- 5400 RPM

//Temperature thresholds
#define mou 2
uint8_t t1;
uint8_t t2;
uint8_t t3;
uint8_t t4=0;

const std::string hwmon = "/sys/class/hwmon";

// harware variables
uint cpu_temp;
uint gpu_temp;
uint cpu_fan;
uint gpu_fan;

// Dellsmm pathes
std::string GPU_path;
std::string CPU_path;
std::string CPU_fan_path;
std::string GPU_fan_path; 
std::string dellsmm="";

bool verbose= false;

//  Gets the needed paths.
void Hwmon_get()
{
    
    for (const auto &entry : fs::directory_iterator(hwmon))
    {
        std::ifstream namepath = std::ifstream(entry.path().string() + "/name");
        std::string name = std::string((std::istreambuf_iterator<char>(namepath)),
                                       (std::istreambuf_iterator<char>()));
        if (name == "dell_smm\n")
        {
            dellsmm = entry.path().string();
            for (const auto & file : fs::directory_iterator(dellsmm)){

                std::ifstream a;
                std::string file_path = file.path().string();
                a.open(file_path);
                std::string sensor_name;
                a >> sensor_name;
                a.close();

                if (sensor_name == "GPU"){
                    GPU_path = file_path;
                    GPU_path.replace(GPU_path.length()-5,5,"input");
                }
                else if (sensor_name =="CPU")
                {
                    CPU_path = file_path;
                    CPU_path.replace(CPU_path.length()-5,5,"input");
                }
                else if (sensor_name =="Processor")
                {
                    CPU_fan_path = file_path;
                    CPU_fan_path.replace(CPU_fan_path.length()-5,5,"input");
                }
                else if (sensor_name =="Video")
                {
                    GPU_fan_path = file_path;
                    GPU_fan_path.replace(GPU_fan_path.length()-5,5,"input");
                }
            }
        }
        // else if (name == "zenpower\n")
        // {
        //     CPU = entry.path().string();
        // }
        // else if (name == "k10temp\n")
        // {
        //     CPU = entry.path().string();
        // }
        // else if (name == "amdgpu\n")
        // {
        //     // There are two amd gpus (iGPU and dGPU) an easy way to differentiate them is to check the presence of pwm1. Another way would be to check vbios version (may be more robust).
        //     if (fs::exists(entry.path().string() + "/pwm1"))
        //     {
        //         dGPU = entry.path().string();
        //     }
        // }
    }
    if (dellsmm == ""){
        std::cout << "Cannot find Dell-smm-hwmon. Try 'modprobe dell-smm-hwmon restricted=0 ignore_dmi=1'. " << std::endl;
        exit(EXIT_FAILURE);
    }
};

// Updates the thermals and fan variables.
void update_vars()
{
    std::ifstream a;
    a.open(CPU_path); //CPU dellsmm
    a >> cpu_temp;
    a.close();
    a.open(GPU_path); //GPU dellsmm
    a >> gpu_temp;
    a.close();
    a.open(CPU_fan_path); //Processor fan
    a >> cpu_fan;
    a.close();
    a.open(GPU_fan_path); // Video fan
    a >> gpu_fan;
    a.close();
};

// Set cpu fans to selected speed. Input should be in the set {0,128,256}.
void set_cpu_fan_smm(int left)
{
    // Force left to be in [0,256]
    int l = std::max(0, std::min(255, left));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    pwm << l;
    pwm.close();
};
// Set gpu fans to selected speed. Input should be in the set {0,128,256}.
void set_gpu_fan_smm(int right)
{
    // Force right to be in [0,256]
    int r = std::max(0, std::min(255, right));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm3");
    pwm << r;
    pwm.close();
};


void write_to_ec(int byte_offset, uint8_t value){
    int fd = open(ECio, O_WRONLY);
    int error;

	error = lseek(fd, byte_offset, SEEK_SET);
	if (error == byte_offset)
		write(fd, &value, 1);
}


void manual_fan_mode(bool on)
{
    if(on){
        write_to_ec(ManualECMode_cpu, 255);
        write_to_ec(ManualECMode_gpu, 255);
    }
    else
    {
        write_to_ec(ManualECMode_cpu, 4);
        write_to_ec(ManualECMode_gpu, 4); 
    }
    if (on)
        printf("Set fans to manual mode.\n");
    else
    {
        printf("Returned to BIOS Fan control.\n");
    }
    
}


// Checks if launched with enough permissions, and requirements are filled.
void permissions_requirements_check()
{
    // Check root permissions.
    if (getuid()!=0){
        std::cout << "No root permissions given."<< std::endl;
        exit(EXIT_FAILURE);
    }
    // Checks if ec_sys is correctly loaded.
    fs::path f(ECio);
    if(!fs::exists(f)){
        std::cout << "Cannot find ECio path. Try 'sudo modprobe ec_sys write_support=1' or add 'ec_sys.write_support=1' as kernel parameter if that's not working."<< std::endl;
        exit(EXIT_FAILURE);
    }
};

void set_cpu_fan(int speed){
    write_to_ec(CPUaddr, speed);
};
void set_gpu_fan(int speed)
{
    write_to_ec(GPUaddr, speed);
};

// Converts hex input to the EC format and limits the max fan speed to boost speed (5400 RPM).
uint8_t hex_to_EC(uint8_t hex){
    return std::min(std::max(255-hex, 91),255);
};

// Prints a status update.
void print_status()
{
    std::cout << "Current fan speeds : " << cpu_fan << " RPM and " << gpu_fan << " RPM.      " << std::endl;
    std::cout << "CPU and GPU temperatures : " << cpu_temp/1000 << "°C and " << gpu_temp/1000 << "°C.  " << std::endl;
    std::cout << "\033[2F";
    if (verbose) std::cout << "\033[1F";
};


int fan_curve(uint8_t current_temp, uint current_fan){
    if (current_temp <t1)
    {
        if(current_temp <t1-mou && current_fan > 1500){
            return ZERO;
        }
        return -1;
    }
    if (current_temp <t2)
    {
        if( (current_temp <t2-mou && current_fan >2500) || current_fan < 1500){
            return SLOW;
        }
        return -1;
    }
    if (current_temp <t3)
    {
        if((current_temp <t3-mou && current_fan > 3500 ) || current_fan < 2500){
            return NORMAL;
        }
        return -1;
    }
    if (current_temp <t4 )
    {
        if((current_temp <t4-mou && current_fan > 4900) || current_fan < 3500){
            return FAST;
        }
        return -1;
    }
    return BOOST;
}

void fan_update(){
    int cpu_update = fan_curve(cpu_temp/1000,cpu_fan);
    if (cpu_update != -1 )
        set_cpu_fan(cpu_update);

    int gpu_update = fan_curve(gpu_temp/1000,gpu_fan);
    if (gpu_update != -1 ) 
        set_gpu_fan(gpu_update);
    if (verbose)
    {
        std::cout <<"CPU and GPU fans update : "<<  cpu_update <<" and "<< gpu_update << ".   "<<std::endl; 
    }
    
}
void usage(char* prog_name, int status){
    printf("Usage :\n");
    printf("sudo %s [-s left_fan_speed right_fan_speed] [-l t1 t2 t3 t4] [-t timer] [-r ] [-b]\n\n", prog_name);
    printf("Arguments description :\n");
    printf(" -h, --help             show this help message and exit.\n");
    printf(" -s, --set left right   sets left/rights fan to selected speed (from 0 to 255).\n");
    printf(" -l, --loop t1 t2 t3 t4 given the temperature thresholds t1, t2, t3, t4 (in °C),\n");
    printf("                        adjusts the fan speed accordingly with a loop.\n");
    printf(" -t, --timer t          set the loop timer (in seconds). Default is 20s.\n");
    printf(" -m, --manual           Switch to manual fan mode.\n");
    printf(" -r, --restore          Gives back the fan control to the BIOS.\n");
    printf(" -b, --boost            Set fan speed to BOOST (as fn+F7).\n");
    printf(" -v, --verbose          Prints loop fan updates. -1 means no fan update made.\n");
    exit(status);
}


void exit_handler(int s){
    // Formating.
    std::cout << std::endl << std::endl;
    if (verbose) std::cout << std::endl;

    // Gives control to the BIOS.
    printf("Exit requested. (Caught signal %d)\n", s);
    manual_fan_mode(false);
    exit(s);
}

int main(int argc, char* argv[])
{
    uint timer=20;
    // Argument Parsing
    for(int i=1; i<argc; i++)
    {
        if (std::string(argv[i])=="--restore" || std::string(argv[i])=="-r")
        {
            permissions_requirements_check();
            manual_fan_mode(false);
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--manual" || std::string(argv[i])=="-m")
        {
            permissions_requirements_check();
            manual_fan_mode(false);
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--boost" || std::string(argv[i])=="-b")
        {
            permissions_requirements_check();
            manual_fan_mode(true);
            set_gpu_fan(BOOST);
            set_cpu_fan(BOOST);
            printf("Boost speed. Be carefull, manual fan mode is on.");
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--set" || std::string(argv[i])=="-s")
        {
            permissions_requirements_check();
            if (argc < i+3)
            {
                printf("Need more arguments.\n");
                exit(EXIT_FAILURE);
            }
            
            manual_fan_mode(true);
            
            uint8_t left = std::stoi(argv[i+1]);
            set_cpu_fan(hex_to_EC(left));
            uint8_t right = std::stoi(argv[i+2]);
            set_gpu_fan(hex_to_EC(right));

            std::cout << "Set fans to "<< std::stoi(argv[i+1]) <<" and " << std::stoi(argv[i+2])<< ". Be carefull, manual fan mode is on."<< std::endl;
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--timer" || std::string(argv[i])=="-t")
        {
            if (argc <i+1)
            {
                exit(EXIT_FAILURE);
            }
            timer = std::stoi(argv[i+1]);
        }
        if (std::string(argv[i])=="--loop" || std::string(argv[i])=="-l")
        {
            permissions_requirements_check();
            if (argc < i+5)
            {
                printf("Need 4 temperature thresholds.\n");
                exit(EXIT_FAILURE);
            }
            t1 = std::stoi(argv[i+1]);
            t2 = std::stoi(argv[i+2]);
            t3 = std::stoi(argv[i+3]);
            t4 = std::stoi(argv[i+4]);
        }
        if (std::string(argv[i])=="--help" || std::string(argv[i])=="-h")
            usage(argv[0],EXIT_SUCCESS);
        if (std::string(argv[i])=="--verbose" || std::string(argv[i])=="-v")
            verbose=true;
        
    }
    if (t4==0) usage(argv[0],EXIT_FAILURE);

    // Set fans to auto once interrupt signal is sent.
    signal (SIGINT,exit_handler);

    // Get hwmon variables
    Hwmon_get();

    // Set fans to manual mode.
    manual_fan_mode(true); // This sets fan to random speeds.
    sleep(1); // May not be needed.
    set_cpu_fan(SLOW);
    set_gpu_fan(SLOW);

    // Fan update loop
    while (true)
    {
        //First update the variables.
        update_vars();
        //Then update the fan speed accordingly.
        fan_update();
        // Prints current status.
        print_status();
        // wait $timer seconds.
        sleep(timer);
    }
    return EXIT_FAILURE;
}

