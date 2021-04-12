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


#include <err.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

#define ECio "/sys/kernel/debug/ec/ec0/io"
#define GPUaddr 151 // 0x97
#define CPUaddr 148 //0x94
#define ZERO 255 // 0xFF
#define SLOW 240 // 0xF0
#define MEDIUM 200 // 204 -- 0xCC
#define NORMAL 163 // 0xA3
#define FAST 102 // 0x66 
#define BOOST 91 // 0x5B

const std::string hwmon = "/sys/class/hwmon";

int cpu_temp;
int gpu_temp;
int cpu_fan;
int gpu_fan;

std::string GPU_path;
std::string CPU_path;
std::string CPU_fan_path;
std::string GPU_fan_path; 
std::string dellsmm="";

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
void set_cpu_fan(int left)
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
void set_gpu_fan(int right)
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
	if (error != byte_offset)
		err(EXIT_FAILURE, "Cannot set offset to 0x%.2x", byte_offset);

	error = write(fd, &value, 1);
	if (error != 1)
		err(EXIT_FAILURE, "Cannot write value 0x%.2x to offset 0x%.2x",
		    value, byte_offset);
}

void check_fan_write_permission()
{
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    if (!pwm.is_open())
    {
        std::cout << "Cannot change fan speed. Are you running the script with root permission ?" << std::endl;
        exit(EXIT_FAILURE);
    }
    pwm.close();
};

// Updates fans accordings to temp.
void update_fans(int lowtemp, int hightemp)
{
    // Handle the left (cpu) fan
    if (cpu_temp < lowtemp)
    {
        if (cpu_fan > 2500 || cpu_fan <1500)
        {
            set_cpu_fan(0);
            // write_to_ec(CPUaddr,SLOW); // Not working...
        }
    }
    else if (cpu_temp < hightemp)
    {
        if (cpu_fan <= 1900 || cpu_fan >= 3500)
        {
            set_cpu_fan(128);
        }
    }
    else
    {
        if (cpu_fan < 3500)
        {
            set_cpu_fan(255);
        }
    }
    // Handles the right (GPU) fan
    if (gpu_temp < lowtemp)
    {
        if (gpu_fan > 1900 || gpu_fan < 1500)
        {
            write_to_ec(GPUaddr,SLOW);
        }
    }
    else if (gpu_temp < hightemp)
    {
        
        if (gpu_fan >= 2900 || cpu_fan >1000)
        {
            write_to_ec(GPUaddr,MEDIUM);
        }
        else if (gpu_fan <= 2100)
        {
            set_gpu_fan(128);
        }
    }
    else
    {
        if (gpu_fan < 3500)
        {
            set_gpu_fan(255);
            // write_to_ec(GPUaddr,FAST);
        }
    }
};

void print_status()
{
    std::cout << "Current fan speeds : " << cpu_fan << " RPM and " << gpu_fan << " RPM.      " << std::endl;
    std::cout << "CPU and GPU temperatures : " << cpu_temp/1000 << "°C and " << gpu_temp/1000 << "°C.  " << std::endl;
    std::cout << "\033[2F";
};

int main(int argc, char* argv[])
{

    if (argc <= 2)
    {
        printf("Need more arguments.\n");
        printf("Usage : DellFan lowtemp hightemp timer\n");
        printf("where 'lowtemp' is your desired idle temperature, and 'hightemp' is the temperature at which fans are set to full speed.\n");
        printf("or, if you only want to set speed once : DellFan leftspeed rightspeed -1\n");
        exit(EXIT_FAILURE);
    }

    const int lowtemp = std::stoi(argv[1]) * 1000;
    const int hightemp = std::stoi(argv[2]) * 1000;
    int timer =10;
    if (argc >3){
        timer = std::stoi(argv[3]);
    }
    // std::cout << "Script launched with arguments : " << lowtemp/1000 << " " << hightemp/1000 << " " << timer <<std::endl; //Debug
    
    // Get hwmon variables
    Hwmon_get();
    // Check if launched with enough permissions.
    check_fan_write_permission();
    
    if(timer <=0 ){
        // In that case, we assume the user only wants to set fans once, and exit.
        const int left = std::min(std::max(lowtemp/1000,0),255); 
        const int right = std::min(std::max(hightemp/1000,0),255);
        set_cpu_fan(left);
        set_gpu_fan(right);
        std::cout << "Set fans to " << left << " and " << right << "."<< std::endl;
        return EXIT_SUCCESS;
    }
    // Fan update loop
    while (true)
    {
        //First update the variables.
        update_vars();
        //Then update the fan speed accordingly.
        update_fans(lowtemp,hightemp);
        //Prints current status
        print_status();
        // wait $timer seconds
        sleep(timer);
    }
    return EXIT_SUCCESS;
}

// void on_window_main_destroy()
// {
//     gtk_main_quit();
// }
