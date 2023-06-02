#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <fcntl.h>
#include <signal.h>

namespace fs = std::filesystem;

// Paths definition.
#define ECio "/sys/kernel/debug/ec/ec0/io"
#define hwmon "/sys/class/hwmon"
//#define temp_thresholds_address "/tmp/._dellfan_tt_ptr"


// Byte offsets
#define ManualECMode_cpu 147 // 0x93
#define ManualECMode_gpu 150 // 0x96
#define GPUaddr 151 // 0x97
#define CPUaddr 148 //0x94

// Hex fan speeds (defaults)
#define ZERO 	255 	// 0xFF -- 0 	RPM		|	0 
#define SLOW 	245 	// 0xF5 -- 2000	RPM		|	10
#define MEDIUM 	196 	// 0xC4 -- 2500	RPM		|	59
#define NORMAL 	162 	// 0xA2 -- 3000	RPM		|	93
#define FAST 	122 	// 0x7A -- 4000	RPM		|	133
#define FASTER 	102 	// 0x66 -- 4800	RPM		|	153
#define BOOST 	91 		// 0x5B -- 5400	RPM		|	164


/////////////////////////////////
// Global variable initialisation
/////////////////////////////////
bool verbose = false;
// Hex fan vector
constexpr uint8_t hex_fan_speeds[] = {ZERO, SLOW, MEDIUM, NORMAL, FAST, FASTER, BOOST};
constexpr uint rpm_fan_speeds[] = {0,2000,2500,3000,4000, 4800,5400};
constexpr uint n_thresholds = sizeof(hex_fan_speeds) / sizeof(uint8_t);

//Temperature thresholds (for fan curve)
unsigned int mou = 2;
uint temp_thresholds[n_thresholds-1] = {45, 50, 55, 60, 70, 80}; // of size n_threshold - 1

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
// default timer.
uint timer=2;


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
			if (verbose)
				std::cout << "Hwmon dell smm path : " <<  dellsmm << "\n";
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


inline void write_to_ec(int byte_offset, uint8_t value){
    int fd = open(ECio, O_WRONLY);
	int error = lseek(fd, byte_offset, SEEK_SET);
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
        std::cout << "No root permissions given.\n";
        exit(EXIT_FAILURE);
    }
    // Checks if ec_sys is correctly loaded.
    fs::path f(ECio);
    if(!fs::exists(f)){
        std::cout << "Cannot find ECio path. Try 'sudo modprobe ec_sys write_support=1' or add 'ec_sys.write_support=1' as kernel parameter if that's not working.\n";
        exit(EXIT_FAILURE);
    }
};

inline void set_cpu_fan(int speed){
    write_to_ec(CPUaddr, speed);
};
inline void set_gpu_fan(int speed)
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

inline uint d(int a, int b) {return std::abs(b-a);}

inline int fan_curve(uint current_temp, uint current_fan){
	uint i = 0;
	// sets i such that : temp_thresholds[i-1] < current_temp < temp_thresholds[i], which we want at speed rpm_fan_speed[i].
	while (i < n_thresholds-1 && current_temp >= temp_thresholds[i]) 
		i++;
	if (i >= n_thresholds -1 && d(current_fan, BOOST) < 400)
		return BOOST;
	if (current_temp >= temp_thresholds[i]-mou && d(current_fan, rpm_fan_speeds[i+1]) < 300) // if close to next threshold, do nothing (prevent fans from changing speed to much)
		return -1;
	else if (d(current_fan, rpm_fan_speeds[i]) > 300) // changes the fan speed if not at the right threshold
		return hex_fan_speeds[i];
	else // do nothing otherwise
		return -1;
}
inline void fan_update(){
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
    printf("sudo %s [-s  left right] [-l] [-ut t1 t2 t3 t4 t5 t6 t7] [-t timer] [-r] [-m] [-b] [-v] \n\n", prog_name);
    printf("Arguments description :\n");
    printf(" -h, --help             show this help message and exit.\n");
    printf(" -s, --set left right   sets left/rights fan to selected speed (from 0 to 164).\n");
	printf(" -l, --loop             launch into loop mode, ie, execute a fan\n");
	printf("                        update every $timer seconds.\n");
    printf(" -ut  t1--t7            sets the temperature thresholds \n");
	printf("                        t1, t2, t3, t4, t5, t6, t7 (in °C),\n");
	printf("                        of the loop update.\n");
    printf(" -t, --timer t          set the loop timer (in seconds). Default is 2s.\n");
    printf(" -m, --manual           Switch to manual fan mode.\n");
    printf(" -r, --restore          Gives back the fan control to the BIOS.\n");
    printf(" -b, --boost            Set fan speed to BOOST (as fn+F7).\n");
    printf(" -v, --verbose          Prints loop fan updates. -1 means no fan update made.\n");
    exit(status);
}


void exit_handler(int s){
    // Formating.
    std::cout << "\n\n";
    if (verbose) std::cout << std::endl;

    // Gives control to the BIOS.
    printf("Exit requested. (Caught signal %d)\n", s);
    manual_fan_mode(false);
	
    exit(s);
}


inline void _timer(int i, int argc, char* argv[]){
	if (argc < i+1)
	{
		exit(EXIT_FAILURE);
	}
	timer = std::stoi(argv[i+1]);
}
inline void _update_thresholds(uint i, uint argc, char* argv[]){
	if (argc < i+n_thresholds)
	{
		printf("Need %i temperature thresholds.\n", n_thresholds-1 );
		exit(EXIT_FAILURE);
	}
	for (unsigned int j=0; j<n_thresholds-1; j++){
		temp_thresholds[j] = std::stoi(argv[i+j+1]);
	}
}
inline void _set(int i, int argc, char* argv[]) {
	permissions_requirements_check();
	if (argc < i+3)
	{
		printf("Need more arguments.\n");
		exit(EXIT_FAILURE);
	}
	manual_fan_mode(true);
	uint8_t left = std::stoi(argv[i+1]);
	set_cpu_fan(hex_to_EC(left));
	uint8_t right;
	if (argc < i+2) // If only one speed is given, set both fans to this speed.
		right = left;
	else
		right = std::stoi(argv[i+2]);
	set_gpu_fan(hex_to_EC(right));
	std::cout << "Set fans to "<< (int)(left) <<" and " << (int)(right) << ". Be careful, manual fan mode is on."<< std::endl;
	exit(EXIT_SUCCESS);
}
inline void _boost(){
	permissions_requirements_check();
	manual_fan_mode(true);
	set_gpu_fan(BOOST);
	set_cpu_fan(BOOST);
	printf("Boost speed. Be carefull, manual fan mode is on.");
	exit(EXIT_SUCCESS);
}
inline void _restore(){
	permissions_requirements_check();
	manual_fan_mode(false);
}
inline void _manual(){
	permissions_requirements_check();
	manual_fan_mode(true);
}


inline bool argument_parser(int argc, char* argv[]){ // returns true if loop has to be made
	bool loop = false;
	if (argc < 2)
		usage(argv[0],EXIT_SUCCESS);
    for(int i=1; i < argc; i++)
    {
		std::string str = std::string(argv[i]);
		if (str=="--help" || str=="-h")
            usage(argv[0],EXIT_SUCCESS);
        else if (str=="--verbose" || str=="-v")
            verbose=true;
        else if (str=="--restore" || str=="-r")
        {
			_restore();
			exit(EXIT_SUCCESS);
        }
        else if (str=="--manual" || str=="-m")
        {
            _manual();
			exit(EXIT_SUCCESS);
        }
        else if (str=="--boost" || str=="-b")
        {
            _boost();
			exit(EXIT_SUCCESS);
        }
        else if (str=="--set" || str=="-s"){
			_set(i,argc,argv);
			exit(EXIT_SUCCESS);
		}
        else if (str=="--timer" || str=="-t")
        {
            _timer(i, argc, argv);
        }
        else if (str=="--loop" || str=="-l")
        {
			loop=true;
        }
		else if (str=="--update" || str=="-ut")
		{
			_update_thresholds(i, argc, argv);
		}

    }
	return loop;
}

int main(int argc, char* argv[])
{
    if (argument_parser(argc, argv)){ // true if the loop argument is send
		permissions_requirements_check();
		// Set fans to auto once interrupt signal is sent.
		signal(SIGINT,exit_handler);
		signal(SIGTERM,exit_handler);

		// Get hwmon variables
		Hwmon_get();
		if(verbose) std::cout << "Temperature threshold pointer (uint): " << &temp_thresholds << "\n";
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
	}
    return EXIT_FAILURE;
}

