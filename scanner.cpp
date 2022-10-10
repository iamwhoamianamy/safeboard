#include <iostream>
#include <fstream>
#include <filesystem>

const std::string jsSus = "<script>evil_script()</script>";
const std::string unixSus = "rm -rf ~/Documents";
const std::string macOSSus = "system(\"launchctl load /Library/LaunchAgents/com.malware.agent\")";

enum class SusType
{
    None,
    Js,
    Unix,
    MacOS,
    Error
};

SusType checkForSuspicion(const std::filesystem::path& path)
{
    std::ifstream fin;

    try
    {
        fin.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
        fin.open(path, std::ios::in);

        std::string line;

        while (std::getline(fin, line))
        {
            if (path.string().find(".js") != std::string::npos)
            {
                if (line.find(jsSus) != std::string::npos)
                {
                    return SusType::Js;
                }
            }
            else
            {
                if (line.find(unixSus) != std::string::npos)
                {
                    return SusType::Unix;
                }
                else if (line.find(macOSSus) != std::string::npos)
                {
                    return SusType::MacOS;
                }
            }
        }        
    }
    catch(const std::exception& e)
    {
        if(!fin.eof())
        {
            return SusType::Error;
        }
    }

    return SusType::None;
}

struct ScanResults
{
    size_t jsFiles = 0;
    size_t unixFiles = 0;
    size_t macOSFiles = 0; 
    size_t errors = 0;
    size_t proccessed = 0;

    void add(SusType type)
    {
        switch (type)
        {
            case SusType::Js: jsFiles++; break;
            case SusType::Unix: unixFiles++; break;
            case SusType::MacOS: macOSFiles++; break;
            case SusType::Error: errors++; break;
            default: break;
        }

        proccessed++;
    }
};

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Not enough params! Example:" << std::endl;
        std::cout << argv[0] << " " << "/directory" << std::endl;
        return 1;
    }

    try
    {
        std::filesystem::path path(argv[1]);
        ScanResults scanResult;        

        auto start = std::chrono::steady_clock::now();

        for (const auto& file : std::filesystem::directory_iterator(path))
        {
            SusType sus = checkForSuspicion(file.path());
            scanResult.add(sus);
        }
        
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() / 1000.0;

        std::cout << "====== Scan result ======" << std::endl;
        std::cout << "Processed files: " << scanResult.proccessed << std::endl;
        std::cout << "JS detects: " << scanResult.jsFiles << std::endl;
        std::cout << "Unix detects: " << scanResult.unixFiles << std::endl;
        std::cout << "macOS detects: " << scanResult.macOSFiles << std::endl;
        std::cout << "Errors: " << scanResult.errors << std::endl;
        std::cout << "Exection time: " << time << std::endl;
        std::cout << "=========================" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << '\n';
    }
    
    return 0;
}