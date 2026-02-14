#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

using namespace std;
#include "ModZ.h"
#include <string>


class Timer{
    public:
        Timer(){
            m_StartTimePoint = std::chrono::high_resolution_clock::now();
        }
        ~Timer(){
            Stop();
        }
        void Stop(){
            auto endTimePoint = std::chrono::high_resolution_clock::now();

            auto start = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimePoint).time_since_epoch().count();
            auto end = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimePoint).time_since_epoch().count();

            auto duration = end - start;
            double s = duration * 0.000001;

            std::cout <<"function took " << duration << "ns (" << s <<"s) to execute\n";
        }
    private:
        std::chrono::time_point< std::chrono::high_resolution_clock >m_StartTimePoint;

};

struct SystemStats {
    float mem = 0.0f;
    float cpu = 0.0f;
    float temp = 0.0f;
};

std::atomic<bool> running(true);
std::mutex stat_mutex;
SystemStats latestStats;

// Your memory usage function
float getMemoryUsagePercent() {
    std::ifstream meminfo("/proc/meminfo");
    std::string label;
    long memTotal = 0, memAvailable = 0;
    while (meminfo >> label) {
        if (label == "MemTotal:") meminfo >> memTotal;
        else if (label == "MemAvailable:") meminfo >> memAvailable;
        meminfo.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (memTotal == 0) return 0.0f;
    long memUsed = memTotal - memAvailable;
    return (float)memUsed / memTotal * 100.0f;
}

// CPU and temp as already written
// For CPU, must keep static prev for sampling
struct CpuTimes {
    unsigned long long idle = 0;
    unsigned long long total = 0;
};

CpuTimes readCpuTimes() {
    std::ifstream stat("/proc/stat"); 
    std::string cpu;
    CpuTimes times{};
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    stat >> cpu >> user >> nice >> system >> idle>> iowait >> irq >> softirq >> steal;
    times.idle = idle + iowait;
    times.total = user + nice + system + idle + iowait + irq + softirq + steal;
    return times;
}

float getCpuUsagePercent() {
    static CpuTimes prev = readCpuTimes();
    CpuTimes curr = readCpuTimes();
    unsigned long long idleDelta  = curr.idle  - prev.idle;
    unsigned long long totalDelta = curr.total - prev.total;
    prev = curr;
    if (totalDelta == 0) return 0.0f;
    return 100.0f * (1.0f - (float)idleDelta / totalDelta);
}

float getCpuTemperatureC() {
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp"); 
    if (!tempFile.is_open()) return -1.0f;
    int tempMilliC;
    tempFile >> tempMilliC;
    return tempMilliC / 1000.0f;
}

void sample_memory(int ms) {
    while (running) {
        float mem = getMemoryUsagePercent();
        {
            std::lock_guard<std::mutex> lock(stat_mutex);
            latestStats.mem = mem;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

void sample_cpu(int ms) {
    while (running) {
        float cpu = getCpuUsagePercent();
        {
            std::lock_guard<std::mutex> lock(stat_mutex);
            latestStats.cpu = cpu;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

void sample_temp(int ms) {
    while (running) {
        float temp = getCpuTemperatureC();
        {
            std::lock_guard<std::mutex> lock(stat_mutex);
            latestStats.temp = temp;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}
//---------------------------------------------------------------------------------------
// Main thread: print snapshot every 200ms
int main() {

    //-- offline data -- 
    //string CPU_usage_location = "/home/pomme/capstone/data/benigh/log_figure8.csv" ;
    string CPU_usage_location = "/home/pomme/capstone/data/attack/log_figure8_attack.csv";    
    
    ifstream file_cpu(CPU_usage_location);
    vector<float> time;
    vector<float> cpu_temp;
    vector<float> cpu_usage;
    vector<float> cpu_mem;

    vector<float> time_out;
    vector<float> cpu_out;
    vector<float> mz;

    
    if (!file_cpu.is_open()) { // Check if the file failed to open
        std::cerr << "Error: Unable to open file '" << CPU_usage_location << "'" << std::endl;
        return 1; 
    }
    read_CSV(file_cpu, time, cpu_temp, cpu_usage, cpu_mem);
    cout<< cpu_temp.size() <<" test\n";
    //--------------------

    Stat_stuff sampling;
    int num_samples = 12000;
    float T_time = 0; 
    float prev_time = 0;
    //offline loop
    for (int i = 0; i < int(cpu_usage.size()); ++i) {
        if(!sampling.Data_Sample_compleate) {
            sample(cpu_usage[i],num_samples,sampling);
        }
        else{
            if(Mod_Z(cpu_usage[i],sampling) >5.7f){
                if((time[i]-prev_time)> 0.2){
                    T_time = 0;
                }else{
                    T_time += time[i]-prev_time;
                    if(T_time >1.5){
                        cpu_out.push_back(cpu_usage[i]);
                        time_out.push_back(time[i]);
                        mz.push_back(Mod_Z(cpu_usage[i],sampling)); 
                    }
                }
                prev_time = time[i];
                //cout << "value: " << cpu_usage[i] << " Time: " << time[i] << "\n";
            }
        }
    }
    write_csv("out.csv",time_out,cpu_out,mz);
    //write_csv("out.csv",time,cpu_usage);

    if(false){
    int interval = 200; // ms

    std::thread mem_thread(sample_memory, interval);
    std::thread cpu_thread(sample_cpu, interval);
    std::thread temp_thread(sample_temp, interval);

    for (int i = 0; i < 10; ++i) {
        {
            Timer timer;
            std::lock_guard<std::mutex> lock(stat_mutex);
        }
            std::cout << "Memory: " << latestStats.mem << "%, "
                      << "CPU: " << latestStats.cpu << "%, "
                      << "Temp: " << latestStats.temp << "C\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }

    running = false;
    mem_thread.join();
    cpu_thread.join();
    temp_thread.join();

    return 0;
    }
    
}