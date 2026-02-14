#ifndef ModZ_H
#define ModZ_H

#include <vector>
#include <utility> // for std::pair
#include <iostream>
#include <fstream>



float mean(const std::vector<float>& data);
float median(std::vector<float> data);

struct Stat_stuff {
    float med = 0;
    float mad = 0;
    std::vector<float> sample_data;
    bool Data_Sample_compleate = false;
};

void sample(float value,int num_samples,Stat_stuff &sample);
float Mod_Z (float value, Stat_stuff &sample);
float attack_detec(float value, float time, float &prev_t);

//------------------------------------------------------------------
void read_CSV(std::ifstream& file, 
                    std::vector<float>& time, 
                    std::vector<float>& cpu_temp, 
                    std::vector<float>& cpu_usages,
                    std::vector<float>& cpu_mem);

void write_csv(const std::string& filename,
               const std::vector<float>& col1,
               const std::vector<float>& col2);

void write_csv(const std::string& filename,
               const std::vector<float>& col1,
               const std::vector<float>& col2,
               const std::vector<float>& col3);
#endif