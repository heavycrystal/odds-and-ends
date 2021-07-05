/*  Gradiente: A simple C++/C program to generate arbitrary sized wallpapers with simple gradient patterns, using a random number seed.
    Developed by Kevin Biju Kizhake Kanichery in 2020. */

#define DEFAULT_IMAGE_LENGTH 3840
#define DEFAULT_IMAGE_WIDTH 2160
#define DEFAULT_OUTPUT_FILENAME "image.ppm"
// A signal handler catches SIGINT and SIGTERM and ensures the unfinished image file is deleted before exiting safely. This can be turned off by changing the below value to 0. 
#define ENSURE_CLEAN_EXIT 1
// The very rudimentary algorithm divides the image into sectors, then allocates basepoints in the corner 4 sectors. The granularity of these corners can be tweaked by the following two options.
#define LENGTH_SPLIT 4
#define WIDTH_SPLIT 4
// The dropoff radius is the distance at which a basepoint does not affect the pixel colour anymore. Values too low or too high can cause large black or white patches in the final image.
#define MIN_DROPOFF_RADIUS 3*std::max(length, width)/4
#define MAX_DROPOFF_RADIUS std::max(length, width)
// To ensure some amount of non-uniformity in the image. The program is set to disregard two basepoints with similar colour values. The threshold for similarity is just a simple RGB value 
// and can be adjusted below. Setting to zero will remove the similarity checks.
// WARNING: setting it too high may cause the program to hang in search of a matching basepoint. There are no checks against it.
#define SIMILARITY_THRESHOLD 50
#define MIN_CHANNEL_VALUE 0
#define MAX_CHANNEL_VALUE 255
#define MIN_RED_VALUE 0
#define MAX_RED_VALUE 200
#define MIN_GREEN_VALUE 0
#define MAX_GREEN_VALUE 200
#define MIN_BLUE_VALUE 0
#define MAX_BLUE_VALUE 200
#define CHECK_IF_EXISTS 1
#define ENABLE_DEBUG 0

#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#if CHECK_IF_EXISTS
#include <filesystem>
#endif
#include <climits>
#include <chrono>
#include <cmath>
#include <csignal>

struct point
{
    int16_t red;
    int16_t green;
    int16_t blue;
};

struct basepoint
{
    uint64_t length;
    uint64_t width;
    int16_t red;
    int16_t green;
    int16_t blue;
    double dropoff;
};

#if !ENABLE_DEBUG
extern "C" void sig_handler(int signum);
#endif
struct basepoint basepoint_layout_helper(uint64_t length_l, uint64_t length_r, uint64_t width_u, uint64_t width_r, std::vector<struct basepoint> basepoints);
struct point compute_color(uint64_t length, uint64_t width, std::vector<struct basepoint> basepoints);
double compute_absdistance(uint64_t length1, uint64_t width1, uint64_t length2, uint64_t width2);
int16_t main_helper_verifybounds_int16_t(int16_t check);

std::mt19937 engine;
std::random_device hrng;
int signal_flag = 0;
bool can_handle_interrupt = false;
bool sigint_trigger = false;
uint64_t length;
uint64_t width;

int main(void)
{


#if !ENABLE_DEBUG
    signal(SIGINT, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGABRT, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sig_handler);
#endif    


    std::string input;


    std::cout << "Enter seed [initialize with hardware entropy source]: ";
    std::getline(std::cin, input);
    if(input.empty())
    {
        engine.seed(hrng());
    }
    if(!input.empty())
    {
        try
        {
            engine.seed(std::stoull(input));
        }
        catch(const std::invalid_argument&)
        {
            std::cerr << "Invalid input: Seed must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }
        catch(const std::out_of_range&)
        {
            std::cerr << "Invalid input: Seed must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }        
        input.clear();
    }


    std::cout << "Enter length of output image [" << DEFAULT_IMAGE_LENGTH << "]: " ;
    std::getline(std::cin, input);
    if(input.empty())
    {
        length = DEFAULT_IMAGE_LENGTH;
    }
    if(!input.empty())
    {
        try
        {
            length = std::stoull(input);
        }
        catch(const std::invalid_argument&)
        {
            std::cerr << "Invalid input: Length must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }
        catch(const std::out_of_range&)
        {
            std::cerr << "Invalid input: Length must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }        
        input.clear();        
    }


    std::cout << "Enter width of output image [" << DEFAULT_IMAGE_WIDTH << "]: " ;
    std::getline(std::cin, input);
    if(input.empty())
    {
        width = DEFAULT_IMAGE_WIDTH;
    }
    if(!input.empty())
    {
        try
        {
            width = std::stoull(input);
        }
        catch(const std::invalid_argument&)
        {
            std::cerr << "Invalid input: Width must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }
        catch(const std::out_of_range&)
        {
            std::cerr << "Invalid input: Width must be a positive integer less than " << UINT64_MAX << ". Program will now exit.\n";
            input.clear();
            return 1;
        }    
        input.clear();        
    }


    std::cout << "Enter filename of output image [" <<  DEFAULT_OUTPUT_FILENAME << "]: ";
    std::getline(std::cin, input);
    if(input.empty())
    {
        input = DEFAULT_OUTPUT_FILENAME;
    }


#if CHECK_IF_EXISTS
    if(std::filesystem::exists(input))
    {
        std::string check;
        std::cerr << "Filename entered already exists in current folder! If you want to overwrite, type YES(all uppercase):";
        std::getline(std::cin, check);
        if(check != "YES")
        {
            std::cerr << "Program will now exit.\n";
            return 3;
        }
    }
#endif


    std::vector<struct basepoint> basepoints;
    std::chrono::time_point start_time = std::chrono::system_clock::now();   


    struct basepoint temp;
    temp = basepoint_layout_helper(0, length/LENGTH_SPLIT, 0, width/WIDTH_SPLIT, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(length - length/LENGTH_SPLIT, length, 0, width/WIDTH_SPLIT, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(0, length/LENGTH_SPLIT, width - width/WIDTH_SPLIT, width, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(length - length/LENGTH_SPLIT, length, width - width/WIDTH_SPLIT, width, basepoints);
    basepoints.push_back(temp);
           
    
    can_handle_interrupt = true;     
    std::ofstream image;
    image.open(input, std::ios::in | std::ios::out | std::ios::trunc);


    image << "P3\n";
    image << length << "\n";
    image << width << "\n";
    image << MAX_CHANNEL_VALUE << "\n";
    for(uint64_t i = 0; i < width; i++)
    {
        for(uint64_t j = 0; j < length; j++)
        {
            struct point temp = compute_color(j, i, basepoints);
            image << temp.red << " " << temp.green << " " << temp.blue << "\n";
#if ENSURE_CLEAN_EXIT            
            if(signal_flag == SIGINT)
            {
                image.close();
                std::cerr << "An interrupt signal(SIGINT, 2) was received. Unfinished output file will be deleted. Program will now exit.\n";
                std::remove(input.c_str());
                return 2;
            }
            if(signal_flag == SIGTERM)
            {
                image.close();
                std::cerr << "A termination signal(SIGTERM, 15) was received. Unfinished output file will be deleted. Program will now exit.\n";
                std::remove(input.c_str());
                return 15;
            }
#endif            
        }
    }
    image.close();
    std::chrono::time_point end_time = std::chrono::system_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Complete!\n";
    std::cout << "Time elapsed: " << duration.count() << " milliseconds.\n";

#if ENABLE_DEBUG
    std::cout << "Basepoints in image: " << basepoints.size() << "\n";
    for(uint64_t i = 0; i < basepoints.size(); i++)
    {
        std::cout << basepoints.at(i).length << " " << basepoints.at(i).width << "\n";
    }
#endif
    return 0;
}


struct point compute_color(uint64_t length, uint64_t width, std::vector<struct basepoint> basepoints)
{
    struct point temp;
    temp.red = 0;
    temp.green = 0;
    temp.blue = 0;
    for(uint64_t i = 0; i < basepoints.size(); i++)
    {
        if((basepoints.at(i).length == length) && (basepoints.at(i).width == width))
        {
            temp.red = basepoints.at(i).red;
            temp.green = basepoints.at(i).green;
            temp.blue = basepoints.at(i).blue;
            return temp;
        }
            temp.red = temp.red + main_helper_verifybounds_int16_t((double)basepoints.at(i).red*(1.0 - ((1.0/basepoints.at(i).dropoff)*compute_absdistance(length, width, basepoints.at(i).length, basepoints.at(i).width))));  
            temp.green = temp.green + main_helper_verifybounds_int16_t((double)basepoints.at(i).green*(1.0 - ((1.0/basepoints.at(i).dropoff)*compute_absdistance(length, width, basepoints.at(i).length, basepoints.at(i).width))));
            temp.blue = temp.blue + main_helper_verifybounds_int16_t((double)basepoints.at(i).blue*(1.0 - ((1.0/basepoints.at(i).dropoff)*compute_absdistance(length, width, basepoints.at(i).length, basepoints.at(i).width))));         
    }
    if(temp.red > 255)
    {
        temp.red = 255;
    }   
    if(temp.green > 255)
    {
        temp.green = 255;
    }   
    if(temp.blue > 255)
    {
        temp.blue = 255;
    }
#if ENABLE_DEBUG
    uint64_t counter;         
    if(temp.red == 0 && temp.green == 0 && temp.blue == 0)
    {
        std::cerr << "Warning! Uncovered point at " << length << ", " << width << "\n";
        counter = counter + 1;
    }
    std::cerr << "Uncovered points: " << counter  << "\n";
#endif
    return temp;
}


double compute_absdistance(uint64_t length1, uint64_t width1, uint64_t length2, uint64_t width2)
{   
    double temp;
    uint64_t dlength = std::max(length1, length2) - std::min(length1, length2);
    uint64_t dwidth = std::max(width1, width2) - std::min(width1, width2);
    temp = pow(dlength, 2) + pow(dwidth, 2);
    temp = pow(temp, 0.5);
    return temp;
}


int16_t main_helper_verifybounds_int16_t(int16_t check)
{
    if(check > 0)
    {
        return check;
    }
    if(check <= 0)
    {
        return 0;
    }
    std::cerr << "Detected potential bug or error in function main_helper_verifybounds_int16_t. Program cannot continue to operate. Exiting now.\n";
    exit(17);
}


struct basepoint basepoint_layout_helper(uint64_t length_l, uint64_t length_r, uint64_t width_u, uint64_t width_d, std::vector<struct basepoint> basepoints)
{
    struct basepoint temp;


    std::uniform_int_distribution<uint64_t> rand_length(length_l, length_r);
    std::uniform_int_distribution<uint64_t> rand_width(width_u, width_d);
    if(((std::max(MIN_RED_VALUE, MIN_CHANNEL_VALUE) == MIN_CHANNEL_VALUE) && (MIN_CHANNEL_VALUE != MIN_RED_VALUE)) || ((std::min(MAX_RED_VALUE, MAX_CHANNEL_VALUE) == MAX_CHANNEL_VALUE) && (MAX_CHANNEL_VALUE != MAX_RED_VALUE)))
    {
        std::cerr << "Warning! Overriding channel values for red with global channel values. Please check compile time options!\n";
    }
    std::uniform_int_distribution<int16_t> rand_red(std::max(MIN_RED_VALUE, MIN_CHANNEL_VALUE), std::min(MAX_RED_VALUE, MAX_CHANNEL_VALUE));
    if(((std::max(MIN_GREEN_VALUE, MIN_CHANNEL_VALUE) == MIN_CHANNEL_VALUE) && (MIN_CHANNEL_VALUE != MIN_GREEN_VALUE)) || ((std::min(MAX_GREEN_VALUE, MAX_CHANNEL_VALUE) == MAX_CHANNEL_VALUE) && (MAX_CHANNEL_VALUE != MAX_GREEN_VALUE)))
    {
        std::cerr << "Warning! Overriding channel values for green with global channel values. Please check compile time options!\n";
    }
    std::uniform_int_distribution<int16_t> rand_green(std::max(MIN_GREEN_VALUE, MIN_CHANNEL_VALUE), std::min(MAX_GREEN_VALUE, MAX_CHANNEL_VALUE));
    if(((std::max(MIN_BLUE_VALUE, MIN_CHANNEL_VALUE) == MIN_CHANNEL_VALUE) && (MIN_CHANNEL_VALUE != MIN_BLUE_VALUE)) || ((std::min(MAX_BLUE_VALUE, MAX_CHANNEL_VALUE) == MAX_CHANNEL_VALUE) && (MAX_CHANNEL_VALUE != MAX_BLUE_VALUE)))
    {
        std::cerr << "Warning! Overriding channel values for blue with global channel values. Please check compile time options!\n";
    }
    std::uniform_int_distribution<int16_t> rand_blue(std::max(MIN_BLUE_VALUE, MIN_CHANNEL_VALUE), std::min(MAX_BLUE_VALUE, MAX_CHANNEL_VALUE));        
    std::uniform_real_distribution<double> rand_dropoff(MIN_DROPOFF_RADIUS, MAX_DROPOFF_RADIUS);


    temp.length = rand_length(engine);
    engine.discard(temp.length);
    temp.width =  rand_width(engine);
    engine.discard(temp.width);
    temp.red = rand_red(engine);
    engine.discard(temp.red);
    temp.green = rand_green(engine);
    engine.discard(temp.green);
    temp.blue = rand_blue(engine);
    engine.discard(temp.blue);
    temp.dropoff = rand_dropoff(engine);
    engine.discard(MAX_DROPOFF_RADIUS);

    if(SIMILARITY_THRESHOLD)
    {
        for(uint64_t i = 0; i < basepoints.size(); i++)
        {
            if((abs(temp.red - basepoints.at(i).red) < SIMILARITY_THRESHOLD) && (abs(temp.green - basepoints.at(i).green) < SIMILARITY_THRESHOLD) && (abs(temp.blue - basepoints.at(i).blue) < SIMILARITY_THRESHOLD))
            {
                temp = basepoint_layout_helper(length_l, length_r, width_u, width_d, basepoints);
            }
        }
    }    


    return temp;
}


#if !ENABLE_DEBUG
extern "C" void sig_handler(int signum)
{
    if(signum == SIGINT)
    {   
        if(sigint_trigger == true)
        {
            if(can_handle_interrupt == true)
            {
                signal_flag = SIGINT;
            }
            if(can_handle_interrupt == false)
            {
                exit(SIGINT);
            }
        }   
        if(sigint_trigger == false)
        {
            fprintf(stderr, "An interrupt signal was received. Press Ctrl-C again if you want to terminate the program.\n");
            sigint_trigger = true;
            
        }
    }
    if(signum == SIGILL)
    {
        fprintf(stderr, "An abnormal signal(SIGILL, 4) was received. This signifies the program made an illegal instruction. The program will now exit abnormally.\n");
        exit(SIGILL);
    }    
    if(signum == SIGABRT)
    {
        fprintf(stderr, "An abnormal signal(SIGABRT, 6) was received. This signifies an abnormal unspecified abort condition. The program will now exit abnormally.\n");
        exit(SIGABRT);
    } 
    if(signum == SIGFPE)
    {
        fprintf(stderr, "An abnormal signal(SIGFPE, 8) was received. This signifies an erroneous arithmetic operation. The program will now exit abnormally.\n");
        exit(SIGFPE);
    }   
    if(signum == SIGSEGV)
    {
        fprintf(stderr, "An abnormal signal(SIGSEGV, 11) was received. This signifies an attempt to access storage in an invalid or unsafe way. The program will now exit abnormally.\n");
        exit(SIGSEGV);
    }         
    if(signum == SIGTERM)
    {
        if(can_handle_interrupt == true)
        {
            signal_flag = SIGTERM;
        }
        if(can_handle_interrupt == false)
        {
            exit(SIGTERM);
        }
    }
}
#endif