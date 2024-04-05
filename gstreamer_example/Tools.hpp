#pragma once


#include <vector>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <exception>
#include <cstdio>

#include <chrono>
#include <iostream>
#include <thread>

class Tools {
public :
    static std::vector<std::string> getAllFilesInFolder(const std::string& folderPath) {
        std::vector<std::string> filePaths;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                if (entry.is_regular_file()) {
                    filePaths.push_back(entry.path().string());
                }
            }
        }
        catch (std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << '\n';
        }
        catch (std::exception& e) {
            std::cerr << "General error: " << e.what() << '\n';
        }
        return filePaths;
    }

    static std::string getFileContents(const char* filename)
    {
        std::ifstream in(filename, std::ios::in | std::ios::binary);
        if (in)
        {
            std::string contents;
            in.seekg(0, std::ios::end);
            contents.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&contents[0], contents.size());
            in.close();
            return(contents);
        }
        throw std::exception(__func__);
    }

    static std::vector<std::string> getAndLoadFiles(const std::string& folderPath)
    {
        std::vector<std::string> frames;
        auto filesToLoad = getAllFilesInFolder(folderPath);
        try {
            for (const auto& file : filesToLoad)
            {
                std::cout << file << std::endl;
                frames.push_back(getFileContents(file.c_str()));
            }
        }
        catch (std::exception& e) {
            std::cerr << "General error loading image: " << e.what() << '\n';
        }
        return frames;
    }


};