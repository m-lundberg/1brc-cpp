#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <limits>

// Thanks, I hate it
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using namespace std::chrono;

struct Station {
    double sum = 0;
    uint64_t count = 0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
};

int main(int argc, const char* argv[])
{
    // Shut up warnings!
    argc; argv;

    SetConsoleOutputCP(CP_UTF8);

    auto start = steady_clock::now();

    std::ifstream fs("measurements.txt", std::ios::binary);
    std::map<std::string, Station> stations;

    for (std::string line; std::getline(fs, line);) {
        size_t delim_pos = line.find(';');
        std::string_view name(line.data(), delim_pos);
        std::string_view value(line.data() + delim_pos + 1);

        double v;
        std::from_chars(value.data(), value.data() + value.size(), v);

        auto& station = stations[std::string(name)];
        station.sum += v;
        station.count++;
        station.min = std::min(v, station.min);
        station.max = std::max(v, station.max);
    }

    std::cout << '{';
    const char* delim = "";
    for (const auto& [name, station] : stations) {
        std::cout << std::format("{}{}={:.1f}/{:.1f}/{:.1f}", delim, name, station.min, station.sum / station.count, station.max);
        delim = ", ";
    }
    std::cout << "}\n\n";
    std::cout << std::format("Elapsed time: {}\n", duration_cast<milliseconds>(steady_clock::now() - start));

    return 0;
}
