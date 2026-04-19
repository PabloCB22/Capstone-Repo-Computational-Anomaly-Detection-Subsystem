#include <algorithm>
#include <vector>
#include <cmath>
#include <chrono>
#include "ModZ.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <curl/curl.h>
#include <numeric>

using namespace std;
//helper function
float median(std::vector<float> data) {
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 0)
        return (data[n/2 - 1] + data[n/2]) / 2.0;
    else
        return data[n/2];
}

 // API call to UI
bool send_alert(const std::string& message,
                const std::string& severity,
                const std::string& source,
                const std::string& type,
                int confidence,
                const std::string& data_json) {
    const char* token = std::getenv("INTERNAL_ALERT_TOKEN");
    const char* url   = std::getenv("RTCT_INTERNAL_ALERT_URL");
 
    if (!token) {
        std::cerr << "INTERNAL_ALERT_TOKEN not set\n";
        return false;
    }
 
    std::string endpoint = url ? url : "http://api:4000/internal/alert";
 
    std::string payload =
        "{"
        "\"message\":\"" + message + "\","
        "\"severity\":\"" + severity + "\","
        "\"source\":\"" + source + "\","
        "\"type\":\"" + type + "\","
        "\"confidence\":" + std::to_string(confidence) + ","
        "\"data\":" + data_json +
        "}";
 
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl\n";
        return false;
    }
 
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string token_header = std::string("x-internal-token: ") + token;
    headers = curl_slist_append(headers, token_header.c_str());
 
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
 
    CURLcode res = curl_easy_perform(curl);
 
    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
 
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
 
    if (res != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
        return false;
    }
 
    if (status_code < 200 || status_code >= 300) {
        std::cerr << "HTTP error: " << status_code << "\n";
        return false;
    }
 
    return true;
}
 

//-- buff --
// overloaded function to be used to get rid on noisy data
// uses a deque (list) of a certain size 
// after the deque becomes the certain size it calcs the averages
// if a new point gets seen, pops the old and pushes the new
// returns the average aftewards


float buff(Stat_stuff &sample,int size, float value ){
    if(int(sample.buffer.size()) != size){
        sample.buffer.push_back(value);
        return 0;
    }else if( (int(sample.buffer.size()) == size) && (sample.avg == 0) ){ 
        sample.avg = std::accumulate(sample.buffer.begin(), sample.buffer.end(), 0.0);
        sample.avg = sample.avg / size;
    }
    sample.avg = sample.avg -(sample.buffer.front()/size) + (value/size);
    sample.buffer.pop_front();
    sample.buffer.push_back(value);
    return sample.avg;
}

float buff(Stat_stuff &sample, float value ){
    int size = 20;
    if(int(sample.buffer_z.size()) != size){
        sample.buffer_z.push_back(value);
        return 0;
    }else if( (int(sample.buffer_z.size()) == size) && (sample.Z_avg == 0) ){ 
        sample.Z_avg = std::accumulate(sample.buffer_z.begin(), sample.buffer_z.end(), 0.0);
        sample.Z_avg = sample.Z_avg / size;
    }
    sample.Z_avg = sample.Z_avg -(sample.buffer_z.front()/size) + (value/size);
    sample.buffer_z.pop_front();
    sample.buffer_z.push_back(value);
    return sample.Z_avg;
}

//Samples data to get the parameters fir analysis
void sample(float value, int num_samples, Stat_stuff &sample,int buffer){
    if(int(sample.sample_data.size()) < num_samples){    //not enough samples
        if(buff(sample,buffer,value) != 0){
           sample.sample_data.push_back(sample.avg);
        } 

    }else{  //enough samples

        // Calculates MAD : Median Absolute Deviation
        sample.med = median(sample.sample_data);
        std::vector<float> abs_devs;
        abs_devs.reserve(sample.sample_data.size());
        for (float x : sample.sample_data)
            abs_devs.push_back(std::fabs(x - sample.med));
        sample.mad = median(abs_devs);

        // --  mean and sample standard deviation calculation --
        //  mean
        float sum = 0.0f;
        for (float x : sample.sample_data)
            sum += x;
        sample.mean = sum / sample.sample_data.size();

        //  sample standard deviation (unbiased, n-1 denominator)
        float sq_sum = 0.0f;
        for (float x : sample.sample_data)
            sq_sum += (x - sample.mean) * (x - sample.mean);
        sample.stdev = std::sqrt(sq_sum / (sample.sample_data.size() - 1));

        sample.Data_Sample_complete = true;
    }
}


//helper functions
bool Mod_Z (float value, Stat_stuff &sample,float threshold){
    float mz = 0.6745f * (value - sample.med) / sample.mad;
    if(buff(sample, mz) >  threshold){
        return true;
    } 
    return false;
}

float Mod_Z (float value, Stat_stuff &sample){
    return 0.6745f * (value - sample.med) / sample.mad;
}


//filtering any data spikes while processing data
bool spike_filter( float time, float &prev_t,float &T_time){
    if((time - prev_t)> 0.5){
        T_time = 0;

    }else{
        T_time += time - prev_t;;
        
        if(T_time > 3.75){
            return true;
        }
    }
    prev_t = time;
    return false;
}



/*
    type:
    1 - CPU Usage   
    2 - Mem% 
    3 - temp 

    after some testing certain values need to be fixed for the best results
    these values are gathered from pervious benign runs 
*/

bool attack_Detection(float value, float time, int num_samples, Stat_stuff &sampling, int type) {
    // Configuration
    const int buf_sz = (type == 1) ? 120 : 20;      // buffer sizes
    const float thresh = (type == 1) ? 5.5f : 3.0f; // modify Z-score threshold
    
            
    // Fast path: still sampling
    if (!sampling.Data_Sample_complete) {
        int samples = (sampling.count != 0) ? 100 : num_samples; 
        sample(value, samples, sampling, buf_sz);
        if(type == 1){
            sampling.mean =50.5153;
            sampling.mad = 5.01726; 
            sampling.med=50.5153;
        }else if(type == 2){
            sample(value, samples, sampling, buf_sz); // resample values
            //sampling.mean =33.603;
            sampling.mad = 0.121775;
            //sampling.med=33.603;
        }else{
            //sampling.mean =33.603;
            sampling.mad = 0.01775;
            //sampling.med=33.603;
        }
        
        return false;
    }
    

    // Detection logic
    float buf_val = buff(sampling, buf_sz, value);
    
    if (!Mod_Z(buf_val, sampling, thresh)) {
        return false;
    }
    
    if (!spike_filter(time, sampling.prev_time, sampling.T_time)) {
        return false;
    }


    // Type 2 specific: adaptive threshold adjustment
    if (type == 2 || type ==3) {
        if (sampling.avg > sampling.max_num) {
            sampling.max_num = sampling.avg;
            sampling.count = 0;
        } else {
            if (++sampling.count >= 500) {
                sampling.sample_data.clear();
                sampling.Data_Sample_complete = false;
                sampling.count = 0;
            }
        }
    }
    
    return true;
}



/*
    type:
    1 - CPU Usage 
    2 - Mem% 
    3 - Temp

    API call with limiters to not spam UI, plus more information on the detection

    one for return to Benighn the other for Attack detected

*/
struct MetricInfo {
    std::string name;
    std::string unit;
    // API call info for anomaly
    std::string alert_title;
    std::string alert_category;
    std::string alert_description;
    std::string alert_json_key;
    // API call info for recovery
    std::string recovery_title;
    std::string recovery_category;
    std::string recovery_description;
    std::string recovery_message;
};

static const std::unordered_map<int, MetricInfo> METRIC_LOOKUP = {
    {1, {"CPU usage", "%", 
         "CPU anomaly detected", "computational anomaly", "CPU Usage abnormal", "CPU",
         "CPU anomaly resolved", "computational anomaly", "CPU Usage normal", "return to normal"}},
    {2, {"CPU memory", "MB",
         "Memory anomaly detected", "computational anomaly", "CPU Memory abnormal", "Memory",
         "Memory anomaly resolved", "computational anomaly", "CPU Memory normal", "return to normal"}},
    {3, {"CPU temp", "°C",
         "Temperature anomaly detected", "computational anomaly", "CPU Temp abnormal", "Temp",
         "Temperature anomaly resolved", "computational anomaly", "CPU Temp normal", "return to normal"}}
};

void API_call(bool trigger, int type, bool &prev, int &falseCount, float time, Stat_stuff &sample) {
    
    float z_score = Mod_Z(sample.avg, sample);
    auto it = METRIC_LOOKUP.find(type);
    
    if (it == METRIC_LOOKUP.end()) {
        prev = trigger;
        return;
    }
    
    const MetricInfo& metric = it->second;
    
    if (!trigger) {
        falseCount++;
    }
    else if (trigger && !prev) {
        if(falseCount >= 200 && z_score < 3) {
            // Return to benign - TIMING VERSION
            auto terminal_start = std::chrono::steady_clock::now();
            
            std::cout << "✓ Return to benign - " << metric.name 
                    << ": " << sample.avg << metric.unit
                    << " at " << time << "s (z-score: " << z_score << ")" << std::endl;
            
            auto terminal_end = std::chrono::steady_clock::now();
            auto terminal_time = std::chrono::duration_cast<std::chrono::microseconds>(terminal_end - terminal_start);
            
            // API Call with timing
            auto api_call_start = std::chrono::steady_clock::now();
            
            // BUILD JSON WITH TIMING DATA
            std::ostringstream json_stream;
            json_stream << std::fixed << std::setprecision(2);
            json_stream << "{"
                        << "\"" << metric.alert_json_key << "\":\"sudden change\","
                        << "\"timestamp\":" << time << ","
                        << "\"z_score\":" << z_score << ","
                        << "\"value\":" << sample.avg << ","
                        << "\"terminal_time_us\":" << terminal_time.count()
                        << "}"; 
            std::string json = json_stream.str();
            
            bool api_success = send_alert(metric.recovery_title, "low", metric.recovery_category, 
                      metric.recovery_description, 1, json);
            
            auto api_call_end = std::chrono::steady_clock::now();
            auto api_call_time = std::chrono::duration_cast<std::chrono::milliseconds>(api_call_end - api_call_start);
            
            
            // Total time from data reception
            if (sample.timing_active) {
                auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(api_call_end - sample.start_time);
                json_stream << ","
                           << "\"total_time_ms\":" << total_time.count()
                           << "}";
                
                std::cout << "  ⏱  Terminal: " << terminal_time.count() << " μs | "
                         << "API: " << api_call_time.count() << " ms | "
                         << "Total: " << total_time.count() << " ms "
                         << (api_success ? "[✓]" : "[✗]") << std::endl << std::endl;
                sample.timing_active = false;
            } else {
                json_stream << "}";
                std::cout << "  ⏱  Terminal: " << terminal_time.count() << " μs | "
                         << "API: " << api_call_time.count() << " ms "
                         << (api_success ? "[✓]" : "[✗]") << std::endl << std::endl;
            }
        }
        else if (falseCount >= 500) {
            // Anomaly detected - TIMING VERSION
            auto terminal_start = std::chrono::steady_clock::now();
            
            std::cout << "⚠ Abnormal spike - " << metric.name 
                      << ": " << sample.avg << metric.unit
                      << " at " << time << "s (z-score: " << z_score << ")" << std::endl;
            
            auto terminal_end = std::chrono::steady_clock::now();
            auto terminal_time = std::chrono::duration_cast<std::chrono::microseconds>(terminal_end - terminal_start);
            
            // API Call with timing
            auto api_call_start = std::chrono::steady_clock::now();
            
            // BUILD JSON WITH TIMING DATA
            std::ostringstream json_stream;
            json_stream << std::fixed << std::setprecision(2);
            json_stream << "{"
                        << "\"" << metric.alert_json_key << "\":\"sudden change\","
                        << "\"timestamp\":" << time << ","
                        << "\"z_score\":" << z_score << ","
                        << "\"value\":" << sample.avg << ","
                        << "\"terminal_time_us\":" << terminal_time.count()
                        << "}"; 
            
            std::string json = json_stream.str();
            
            bool api_success = send_alert(metric.alert_title, "high", metric.alert_category, 
                      metric.alert_description, 1, json);
            
            auto api_call_end = std::chrono::steady_clock::now();
            auto api_call_time = std::chrono::duration_cast<std::chrono::milliseconds>(api_call_end - api_call_start);
            

            
            // Total time from data reception
            if (sample.timing_active) {
                auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(api_call_end - sample.start_time);
                json_stream << ","
                           << "\"total_time_ms\":" << total_time.count()
                           << "}";
                
                std::cout << "  ⏱  Terminal: " << terminal_time.count() << " μs | "
                         << "API: " << api_call_time.count() << " ms | "
                         << "Total: " << total_time.count() << " ms "
                         << (api_success ? "[✓]" : "[✗]") << std::endl << std::endl;
                sample.timing_active = false;
            } else {
                json_stream << "}";
                std::cout << "  ⏱  Terminal: " << terminal_time.count() << " μs | "
                         << "API: " << api_call_time.count() << " ms "
                         << (api_success ? "[✓]" : "[✗]") << std::endl << std::endl;
            }
        }
        
        falseCount = 0;
    }
    
    prev = trigger;
}