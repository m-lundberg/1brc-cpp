#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <limits>
#include <string_view>
#include <string>
#include <thread>
#include <unordered_map>

// Thanks, I hate it
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using namespace std::chrono;

struct Station {
    int64_t sum = 0;
    uint64_t count = 0;
    int64_t min = std::numeric_limits<int64_t>::max();
    int64_t max = std::numeric_limits<int64_t>::lowest();

    void update(int64_t value) {
        sum += value;
        count++;
        min = std::min(value, min);
        max = std::max(value, max);
    }

    void merge(const Station& other) {
        sum += other.sum;
        count += other.count;
        min = std::min(min, other.min);
        max = std::max(max, other.max);
    }
};

static int64_t parse_number(std::string_view str) {
    if (str.empty()) {
        return 0; // whatever for the purposes of this project
    }

    int sign = 1;
    if (str[0] == '-') {
        sign = -1;
        str.remove_prefix(1);
    }

    // Take advantage of the fact that the non-fractional part can only be 1 or 2 numbers
    int64_t result = 0;
    if (str.size() == 3) {
        // E.g. 1.2
        result = (str[0] - '0') * 10;
    } else if (str.size() == 4) {
        // E.g. 12.3
        result = (str[0] - '0') * 100 + (str[1] - '0') * 10;
    }
    result += str.back() - '0';
    return sign * result;
}

std::unordered_map<std::string, Station> process_chunk(const char* data, size_t start, size_t end) {
    std::unordered_map<std::string, Station> result;
    size_t line_start = start;
    for (size_t i = start; i < end; ++i) {
        if (data[i] != '\n') {
            continue;
        }

        // We have read a complete line, process it
        std::string_view line(data + line_start, i - line_start);

        size_t delim_pos = line.find(';');
        std::string_view name(line.data(), delim_pos);
        std::string_view value(line.data() + delim_pos + 1, i - line_start - delim_pos - 1);

        result[std::string(name)].update(parse_number(value));

        line_start = i + 1;
    }

    return result;
}

int main(int argc, const char* argv[]) {
    // Shut up warnings!
    argc; argv;

    SetConsoleOutputCP(CP_UTF8);

    auto start = steady_clock::now();

    // Create a memory-mapped file view
    HANDLE hfile = CreateFileA("measurements.txt", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hfile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file\n";
        return 1;
    }
    HANDLE hmap = CreateFileMappingA(hfile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hmap) {
        std::cerr << "Failed to create file mapping\n";
        CloseHandle(hfile);
        return 1;
    }
    const char* data = static_cast<const char*>(MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0));
    if (!data) {
        std::cerr << "Failed to create file view\n";
        CloseHandle(hmap);
        CloseHandle(hfile);
        return 1;
    }

    LARGE_INTEGER file_size;
    GetFileSizeEx(hfile, &file_size);
    size_t size = static_cast<size_t>(file_size.QuadPart);

    const unsigned num_threads = std::thread::hardware_concurrency();

    // Find chunk boundaries by splitting the data by whole rows
    std::vector<size_t> chunk_starts(num_threads + 1);
    chunk_starts[0] = 0;
    for (unsigned i = 1; i < num_threads; ++i) {
        // Find the approximate position
        size_t start_pos = size * i / num_threads;

        // Advance to next line break
        while (start_pos < size && data[start_pos] != '\n') {
            ++start_pos;
        }
        chunk_starts[i] = start_pos + 1; // One pos past the '\n'
    }
    chunk_starts[num_threads] = size;

    // Process each chunk in its own thread
    std::vector<std::thread> threads;
    std::vector<std::unordered_map<std::string, Station>> partial_results(num_threads);
    for (unsigned i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            partial_results[i] = process_chunk(data, chunk_starts[i], chunk_starts[i + 1]);
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // Merge the results into one final map
    std::unordered_map<std::string, Station> stations;
    for (const auto& partial : partial_results) {
        for (const auto& [name, station] : partial) {
            stations[name].merge(station);
        }
    }

    // Sort output alphabetically
    std::vector<std::string> names;
    names.reserve(stations.size());
    for (const auto& [key, _] : stations) {
        names.push_back(key);
    }
    std::sort(names.begin(), names.end());

    std::cout << '{';
    const char* delim = "";
    for (const auto& name : names) {
        const auto& station = stations[name];
        std::cout << std::format("{}{}={:.1f}/{:.1f}/{:.1f}",
            delim,
            name,
            station.min / 10.0,
            static_cast<double>(station.sum) / station.count / 10.0,
            station.max / 10.0
        );
        delim = ", ";
    }
    std::cout << "}\n\n";

    UnmapViewOfFile(data);
    CloseHandle(hmap);
    CloseHandle(hfile);

    std::cout << std::format("Elapsed time: {}\n", duration_cast<milliseconds>(steady_clock::now() - start));

    return 0;
}
