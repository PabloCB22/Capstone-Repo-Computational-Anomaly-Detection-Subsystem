#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <future>
#include <iomanip>
 
// ROS2
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"  // Changed to lowercase
 
#include "ModZ.h"
 
using std::placeholders::_1;
using std::cout;
 
class DetectorNode : public rclcpp::Node
{
public:
    DetectorNode()
    : Node("detector_node"),
      p1(false), c1(0),
      p2(false), c2(0),
      p3(false), c3(0)
    {
        pi_metrics_sub_ = this->create_subscription<std_msgs::msg::Float32MultiArray>(
            "pi_system_metrics", 10,
            std::bind(&DetectorNode::pi_metrics_callback, this, _1));
 
        RCLCPP_INFO(this->get_logger(), "Detector node started. Listening on pi_system_metrics...");
    }
 
private:
 
    void pi_metrics_callback(const std_msgs::msg::Float32MultiArray & msg)
    {
        // START TIMING: Data received
        auto callback_start = std::chrono::steady_clock::now();
        
        if (msg.data.size() < 4) {
            RCLCPP_WARN(this->get_logger(), "pi_system_metrics message has fewer than 4 fields, skipping");
            return;
        }
        
        float timestamp = msg.data[0];
        float cpu       = msg.data[1];
        float mem       = msg.data[2];
        float temp      = msg.data[3];
        
        // Store start time in each sampling structure
        {
            std::lock_guard<std::mutex> lock1(cpu_mutex_);
            std::lock_guard<std::mutex> lock2(mem_mutex_);
            std::lock_guard<std::mutex> lock3(temp_mutex_);
            
            sampling_cpu.start_time = callback_start;
            sampling_cpu.timing_active = true;
            
            sampling_mem.start_time = callback_start;
            sampling_mem.timing_active = true;
            
            sampling_temp.start_time = callback_start;
            sampling_temp.timing_active = true;
        }
        
        // Launch three parallel tasks
        auto future1 = std::async(std::launch::async, [this, cpu, timestamp]() {
            std::lock_guard<std::mutex> lock(cpu_mutex_);
            API_call(attack_Detection(cpu, timestamp, 1200, sampling_cpu, 1), 
                    1, std::ref(p1), std::ref(c1), timestamp, std::ref(sampling_cpu));
        });
        
        auto future2 = std::async(std::launch::async, [this, mem, timestamp]() {
            std::lock_guard<std::mutex> lock(mem_mutex_);
            API_call(attack_Detection(mem, timestamp, 1900, sampling_mem, 2), 
                    2, std::ref(p2), std::ref(c2), timestamp, std::ref(sampling_mem));
        });
        
        auto future3 = std::async(std::launch::async, [this, temp, timestamp]() {
            std::lock_guard<std::mutex> lock(temp_mutex_);
            API_call(attack_Detection(temp, timestamp, 1400, sampling_temp, 3), 
                    3, std::ref(p3), std::ref(c3), timestamp, std::ref(sampling_temp));
        });
        
        // Wait for all three tasks to complete
        future1.wait();
        future2.wait();
        future3.wait();
        
        // CALCULATE TOTAL CALLBACK TIME (optional logging)
        auto callback_end = std::chrono::steady_clock::now();
        auto callback_duration = std::chrono::duration_cast<std::chrono::microseconds>(callback_end - callback_start);
        
        // Optional: Log total processing time periodically
        /* static int callback_count = 0;
        if (++callback_count % 100 == 0) {
            RCLCPP_INFO(this->get_logger(), "Total callback processing time: %.3f ms", 
                       callback_duration.count() / 1000.0);
        } */
    }
 
    rclcpp::Subscription<std_msgs::msg::Float32MultiArray>::SharedPtr pi_metrics_sub_;
 
    std::mutex cpu_mutex_;
    std::mutex mem_mutex_;
    std::mutex temp_mutex_;
    
    Stat_stuff sampling_cpu;
    Stat_stuff sampling_mem;
    Stat_stuff sampling_temp;
    bool p1;
    int c1;
    bool p2;
    int c2;
    bool p3;
    int c3;
};
 
int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DetectorNode>();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}