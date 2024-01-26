#pragma once
#include "color.h"
#include <vector>
#include <fstream>

#include <mutex>
#include <thread>
class Image
{
public:
    Image()
    {
        width = 0;
        height = 0;
        pixels = std::vector<Color>();
    }

    Image(int width, int height) : width(width), height(height)
    {
        pixels.reserve(width * height);
    }
    ~Image()
    {
    }
    void setPixel(int x, int y, const Color &c)
    {
        pixels[y * width + x] = c;
    }
    Color getPixel(int x, int y) const
    {
        return pixels[y * width + x];
    }
    void fill(const Color &c)
    {
        pixels.resize(width * height, c);
    }
    void save(const std::string &name)
    {
        // //overwrite the file if it exists
        // std::ofstream file(name, std::ios::out | std::ios::trunc);
        // file << "P3"<<std::endl;
        // file << width << " " << height << std::endl;
        // file << "255" << std::endl;
        // for (int i = 0; i < width * height; i++) {
        //     file << Color::scale_color(pixels[i]) << std::endl;
        // }
        // file.close();

        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(num_threads);

        auto save_section = [&](int thread_id, int start, int end)
        {
            std::string section_name = name + "_section_" + std::to_string(thread_id) + ".tmp";
            std::ofstream file(section_name, std::ios::out | std::ios::trunc);

            for (int i = start; i < end; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    file << Color::scale_color(pixels[i * width + j]) << std::endl;
                }
            }
            file.close();
        };

        // Assign work to threads
        int rows_per_thread = height / num_threads;
        for (int t = 0; t < num_threads; t++)
        {
            int start = t * rows_per_thread;
            int end = (t == num_threads - 1) ? height : start + rows_per_thread;

            threads[t] = std::thread(save_section, t, start, end);
        }

        // Wait for all threads to complete their work
        for (auto &t : threads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }

        // Merge temporary files to create the final image file
        std::ofstream file(name, std::ios::out | std::ios::trunc);
        file << "P3" << std::endl;
        file << width << " " << height << std::endl;
        file << "255" << std::endl;

        for (int t = 0; t < num_threads; t++)
        {
            std::string section_name = name + "_section_" + std::to_string(t) + ".tmp";
            std::ifstream section_file(section_name);
            file << section_file.rdbuf();
            section_file.close();
            std::remove(section_name.c_str());
        }

        file.close();
    }
    static Image ReadPPM(const std::string &name)
    {
        std::ifstream file(name, std::ios::in);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file " << name << std::endl;
            exit(1);
        }

        // Read the header information
        std::string header;
        int max_color;
        int width, height;
        file >> header >> width >> height >> max_color;
        if (header != "P3" || max_color != 255)
        {
            std::cerr << "Invalid PPM file " << name << std::endl;
            exit(1);
        }
        Image image(width, height);
        image.name = name;
        // Read the pixel data
        std::vector<Color> pixels(width * height);
        for (int i = 0; i < width * height; i++)
        {
            int r, g, b;
            file >> r >> g >> b;
            pixels[i] = Color(r / 255.0f, g / 255.0f, b / 255.0f);
        }
        file.close();
        // create image
        image.pixels = pixels;
        return image;
    }
    int width, height;
    std::string name;

private:
    std::vector<Color> pixels;
};