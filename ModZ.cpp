#include <algorithm>
#include <vector>
#include <cmath>

#include "ModZ.h"
using namespace std;
#include <iostream>
#include <fstream>

#include <sstream>
#include <string>
#include <map>

float median(std::vector<float> data) {
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 0)
        return (data[n/2 - 1] + data[n/2]) / 2.0;
    else
        return data[n/2];
}


void sample(float value,int num_samples,Stat_stuff &sample){
    if(int(sample.sample_data.size()) < num_samples){    //not enough samples
        sample.sample_data.push_back(value);
    }else{  //enough samples 
        // calculates MAD : Median Absolute Deviation
        sample.med = median(sample.sample_data);
        std::vector<float> abs_devs;

        abs_devs.reserve(sample.sample_data.size());
        for (float x : sample.sample_data)
            abs_devs.push_back(std::fabs(x - sample.med));
        
        sample.mad = median(abs_devs);
        sample.Data_Sample_compleate = true;
    };
}

float Mod_Z (float value, Stat_stuff &sample){
    return 0.6745f * (value - sample.med) / sample.mad;
}

void read_CSV(std::ifstream& file,
              std::vector<float>& time,
              std::vector<float>& cpu_temp,
              std::vector<float>& cpu_usages,
              std::vector<float>& cpu_mem)
{
    std::string line;

    // Read header line
    if (!std::getline(file, line)) return;

    std::vector<std::string> headers;
    std::istringstream header_stream(line);
    std::string header;

    // Assuming comma separated CSV
    while (std::getline(header_stream, header, ',')) {
        headers.push_back(header);
    }

    // Map header names to indices
    std::unordered_map<std::string, int> col_idx;
    for (int i = 0; i < (int)headers.size(); ++i) {
        col_idx[headers[i]] = i;
    }

    // Columns to extract
    const std::string col_time = "Time";
    const std::string col_cpu = "pi_cpu_percent";
    const std::string col_mem = "pi_memory_percent";
    const std::string col_temp = "pi_temperature";

    // Check required columns exist
    if (col_idx.find(col_time) == col_idx.end() ||
        col_idx.find(col_cpu) == col_idx.end() ||
        col_idx.find(col_mem) == col_idx.end() ||
        col_idx.find(col_temp) == col_idx.end())
    {
        std::cerr << "Missing required columns in CSV\n";
        return;
    }

    int idx_time = col_idx[col_time];
    int idx_cpu = col_idx[col_cpu];
    int idx_mem = col_idx[col_mem];
    int idx_temp = col_idx[col_temp];

    // Read each data line
    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::istringstream ss(line);
        std::string field;

        // Split line by comma
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        if ((int)fields.size() <= std::max(std::max(idx_time, idx_cpu), std::max(idx_mem, idx_temp))) {
            // Malformed line
            continue;
        }

        try {
            float t = std::stof(fields[idx_time]);
            float cpu = std::stof(fields[idx_cpu]);
            float mem = std::stof(fields[idx_mem]);
            float temp = std::stof(fields[idx_temp]);

            time.push_back(t);
            cpu_usages.push_back(cpu);
            cpu_mem.push_back(mem);
            cpu_temp.push_back(temp);
        }
        catch (...) {
            // Skip invalid lines
            continue;
        }
    }
    file.close(); 
}

float attack_detec(float value, float time, float &prev_t){
    return 0;
}

#include <iostream>
#include <fstream>
#include <vector>

void write_csv(const std::string& filename,
               const std::vector<float>& col1,
               const std::vector<float>& col2)
{
    if (col1.size() != col2.size()) {
        std::cerr << "Error: Vectors must be the same size.\n";
        return;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file.\n";
        return;
    }

    // Optional header
    file << "Column1,Column2\n";

    for (size_t i = 0; i < col1.size(); ++i) {
        file << col1[i] << "," << col2[i] << "\n";
    }

    file.close();
}
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

void write_csv(const std::string& filename,
               const std::vector<float>& col1,
               const std::vector<float>& col2,
               const std::vector<float>& col3)
{
    if (col1.size() != col2.size() || col1.size() != col3.size()) {
        std::cerr << "Error: Vectors must be the same size.\n";
        return;
    }

    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file.\n";
        return;
    }

    // Header
    file << "Column1,Column2,Column3\n";

    for (size_t i = 0; i < col1.size(); ++i) {
        file << col1[i] << ","
             << col2[i] << ","
             << col3[i] << "\n";
    }
}
