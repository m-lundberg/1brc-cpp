#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdio.h>
#include <string_view>
#include <string>

// Thanks, I hate it
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using namespace std::chrono;

enum class ReadState {
    NAME,
    TEMPERATURE,
};

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

    // Let's raw dog some C; read the entire file into a buffer (we got RAM)
    FILE* f = nullptr;
    fopen_s(&f, "measurements.txt", "rb");
    fseek(f, 0, SEEK_END);
    long long size = _ftelli64(f);
    std::cout << "Allocating " << size << " bytes" << std::endl;
    char* data = new char[size];
    rewind(f);
    fread(data, sizeof(char), size, f);

    // Still keeping track of every weather station in a map
    std::map<std::string, Station> stations;

    // Loop over characters using a state machine
    ReadState state = ReadState::NAME;
    std::string name_buffer;
    std::string value_buffer;
    for (long long i = 0; i < size; ++i) {
        switch (state) {
        case ReadState::NAME: {
            if (data[i] == ';') {
                state = ReadState::TEMPERATURE;
                continue;
            }
            name_buffer += data[i];
            break;
        }
        case ReadState::TEMPERATURE: {
            if (data[i] == '\n') {
                double v;
                std::from_chars(value_buffer.data(), value_buffer.data() + value_buffer.size(), v);

                auto& station = stations[name_buffer];
                station.sum += v;
                station.count++;
                station.min = std::min(v, station.min);
                station.max = std::max(v, station.max);

                // Prepare for next line
                name_buffer.clear();
                value_buffer.clear();
                state = ReadState::NAME;
                continue;
            }
            value_buffer += data[i];
            break;
        }
        }
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
