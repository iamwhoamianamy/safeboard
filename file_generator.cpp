#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

std::string generateRandomString(size_t length)
{
    std::string result;

    for (size_t j = 0; j < length; j++)
    {
        result += (char)(rand() % 26 + 'a');
    }

    return result;
}

void createFile(const std::string& path, bool isJs)
{
    auto fileName = path + "/" + generateRandomString(20) + (isJs ? ".js" : ".txt");
    std::ofstream fout(fileName, std::ios::out);

    for (size_t i = 0; i < 1000; i++)
    {
        for (size_t j = 0; j < 7; j++)
        {
            fout << generateRandomString(10) << " ";
        }

        fout << std::endl;
    }
}

int main(int argc, char** argv)
{
    if(argc != 3)
    {
        std::cout << "Not enough params! Example:" << std::endl;
        std::cout << argv[0] << " FILE_COUNT /directory" << std::endl;
        return 1;
    }
    
    try
    {
        size_t fileCount = atoll(argv[1]);
        std::string path(argv[2]);
        std::filesystem::create_directory(path);

        for (size_t i = 0; i < fileCount; i++)
        {
            createFile(path, rand() % 4 == 0);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}