// bun_sim_parallel.cpp
// Parallel simulation of package installation using OpenMP.
// This version uses OpenMP to process multiple packages in parallel,
// demonstrating speed-ups for tasks that are a mix of I/O and CPU-bound work.
//
// Compile: g++ -O2 -fopenmp -std=c++17 bun_sim_parallel.cpp -o bun_parallel
// Usage: ./bun_parallel <packages_list.txt> <output_dir>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <omp.h>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

// A thread-safe print function to prevent garbled output from multiple threads.
// Uses an OpenMP critical section to ensure only one thread prints at a time.
void sync_print(const std::string& msg) {
    #pragma omp critical
    {
        std::cout << msg << std::endl;
    }
}

// A simple CPU-bound function to simulate processing file contents.
uint64_t checksum_bytes(const std::vector<char>& data) {
    // FNV-1a hash algorithm (64-bit)
    uint64_t h = 1469598103934665603ULL;
    for (char c : data) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// Processes a single package. This function is designed to be called in parallel.
void process_package(const fs::path& pkg_dir, const fs::path& out_dir) {
    int thread_id = omp_get_thread_num();
    std::stringstream log_msg;
    log_msg << "[Thread " << thread_id << "] ==> Starting package " << pkg_dir.filename().string();
    sync_print(log_msg.str());

    auto start = Clock::now();

    // 1. Read manifest (I/O)
    std::ifstream manifest(pkg_dir / "manifest.json", std::ios::binary);
    if (!manifest) {
        log_msg.str("");
        log_msg << "[Thread " << thread_id << "] Error: Cannot open manifest for " << pkg_dir.filename().string();
        sync_print(log_msg.str());
        return;
    }
    std::string mcontents((std::istreambuf_iterator<char>(manifest)),
                           std::istreambuf_iterator<char>());

    fs::path files_dir = pkg_dir / "files";
    if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) return;

    fs::path out_pkg = out_dir / pkg_dir.filename();
    fs::create_directories(out_pkg);

    // 2. Process all files in the package
    for (auto &p : fs::directory_iterator(files_dir)) {
        if (!fs::is_regular_file(p.path())) continue;
        
        // a. Read file (I/O)
        std::ifstream fin(p.path(), std::ios::binary);
        std::vector<char> buf((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());

        // b. Compute checksum (CPU)
        uint64_t cs = checksum_bytes(buf);

        // c. Write file and metadata (I/O)
        fs::path out_file = out_pkg / p.path().filename();
        std::ofstream fout(out_file, std::ios::binary);
        fout.write(buf.data(), buf.size());
        std::ofstream meta(out_pkg / (p.path().filename().string() + ".meta"), std::ios::trunc);
        meta << "checksum:" << cs << "\n";
    }

    // 3. Update a central DB file. This must be serialized to prevent race conditions.
    // An OpenMP critical section ensures that only one thread can execute this block at a time.
    #pragma omp critical
    {
        fs::path dbfile = out_dir / "install_db.txt";
        std::ofstream db(dbfile, std::ios::app);
        db << pkg_dir.filename().string() << " installed by thread " << thread_id << "\n";
    }

    auto end = Clock::now();
    std::chrono::duration<double> dur = end - start;
    
    log_msg.str(""); // Clear the stringstream
    log_msg << "[Thread " << thread_id << "] <== Finished package " << pkg_dir.filename().string()
            << " in " << std::fixed << std::setprecision(4) << dur.count() << "s.";
    sync_print(log_msg.str());
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <packages_list.txt> <output_dir>\n";
        return 1;
    }
    fs::path listfile = argv[1];
    fs::path outdir = argv[2];
    fs::create_directories(outdir);

    std::vector<fs::path> pkg_dirs;
    std::ifstream in(listfile);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        pkg_dirs.push_back(fs::path(line));
    }

    int total_packages = pkg_dirs.size();
    int completed_packages = 0;

    std::cout << "Starting parallel processing of " << total_packages << " packages...\n"
              << "Max threads: " << omp_get_max_threads() << "\n\n";
    
    auto t0 = Clock::now();

    // This is the main parallel loop.
    // #pragma omp parallel for: Distributes the for-loop iterations among threads.
    // schedule(dynamic, 1): Each thread grabs 1 iteration (package) at a time.
    //   'dynamic' is good for when iterations have varying workloads.
    #pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < pkg_dirs.size(); ++i) {
        process_package(pkg_dirs[i], outdir);
        
        // Atomically increment the counter for completed packages.
        // This is a lightweight way to handle a shared counter without a full lock.
        int current_completed;
        #pragma omp atomic update
        completed_packages++;
        #pragma omp atomic read
        current_completed = completed_packages;

        // Log progress. The sync_print is important here to avoid garbled output.
        std::stringstream progress_msg;
        progress_msg << "                                       Progress: "
                     << current_completed << "/" << total_packages << " ("
                     << std::fixed << std::setprecision(1) << (100.0 * current_completed / total_packages) << "%)";
        sync_print(progress_msg.str());
    }

    auto t1 = Clock::now();
    std::chrono::duration<double> dur = t1 - t0;

    std::cout << "\n--------------------------------------------------\n";
    std::cout << "Processed " << total_packages << " packages in "
              << std::fixed << std::setprecision(4) << dur.count()
              << " seconds (parallel, threads=" << omp_get_max_threads() << ").\n";
    std::cout << "--------------------------------------------------\n";

    return 0;
}