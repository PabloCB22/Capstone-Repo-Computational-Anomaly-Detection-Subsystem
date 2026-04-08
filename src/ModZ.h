#ifndef MODZ_H
#define MODZ_H

#include <deque>
#include <vector>
#include <chrono>
#include <string>
#include <unordered_map>

struct Stat_stuff {
    std::deque<float> buffer;
    std::vector<float> sample_data;
    float avg = 0;
    float med = 0;
    float mad = 0;
    float mean = 0;
    float stdev = 0;
    bool Data_Sample_complete = false;
    float prev_time = 0;
    float T_time = 0;
    int count = 0;
    float max_num = 0;
    
    // TIMING MEMBERS
    std::chrono::steady_clock::time_point start_time;
    bool timing_active = false;
};

// Function declarations
float median(std::vector<float> data);
bool send_alert(const std::string& message,
                const std::string& severity,
                const std::string& source,
                const std::string& type,
                int confidence,
                const std::string& data_json);
float buff(Stat_stuff &sample, int size, float value);
void sample(float value, int num_samples, Stat_stuff &sample, int buffer);
bool Mod_Z(float value, Stat_stuff &sample, float threshold);
float Mod_Z(float value, Stat_stuff &sample);
bool spike_filter(float time, float &prev_t, float &T_time);
bool attack_Detection(float value, float time, int num_samples, Stat_stuff &sampling, int type);
void API_call(bool trigger, int type, bool &prev, int &falseCount, float time, Stat_stuff &sample);

#endif // MODZ_H