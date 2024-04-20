#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdio.h>
#include <string_view>
#include <string>
#include <unordered_map>

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

static double pow(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

static double parse_number(const std::string& str) {
    if (str.empty()) {
        return 0; // whatever for the purposes of this project
    }

    const char* data = str.data();
    size_t data_size = str.size();
    int sign = 1;
    if (data[0] == '-') {
        sign = -1;
        ++data;
        --data_size;
    }

    static std::vector<int> v; // static for speed, reuse the same vector every time
    v.clear();
    v.resize(data_size - 2); // only non-fractional part
    int e = 0;
    bool seen_dot = false;
    for (size_t i = 0; i < data_size; ++i) {
        if (data[i] == '.') {
            seen_dot = true;
            continue;
        }
        if (!seen_dot) {
            v[i] = data[i] - 48;
        }
        else {
            e = data[i] - 48;
        }
    }
    
    double result = 0;
    int i = data_size - 2;
    for (auto& num : v) {
        result += num * pow(10, --i);
    }
    result += e * 0.1;
    return result * sign;
}

int main(int argc, const char* argv[]) {
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
    std::cout << "Reading file" << std::endl;
    rewind(f);
    fread(data, sizeof(char), size, f);
    std::cout << std::format("File loaded in {}\n", duration_cast<milliseconds>(steady_clock::now() - start));

    // Still keeping track of every weather station in a map
    std::unordered_map<std::string, Station> stations;

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
                double v = parse_number(value_buffer);

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
