// bun_sim_serial.cpp
// Serial simulation of package installation (C++).
// A simple simulation to demonstrate file I/O and CPU-bound work
// for a package installation process. This is the baseline for
// performance comparison with the parallel version.
//
// Compile: g++ -O2 -std=c++17 bun_sim_serial.cpp -o bun_serial
// Usage: ./bun_serial <packages_list.txt> <output_dir>
// packages_list.txt: each line: <pkg_dir> (pkg_dir contains manifest.json and files/ subdir)
// Example: ./bun_serial packages.txt out_serial

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <iterator>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

// A simple CPU-bound function to simulate processing file contents.
// It calculates a checksum over a vector of bytes.
uint64_t checksum_bytes(const std::vector<char>& data) {
    // FNV-1a hash algorithm (64-bit)
    uint64_t h = 1469598103934665603ULL;
    for (char c : data) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// Processes a single package serially.
// Returns the time taken in seconds.
double process_package_serial(const fs::path& pkg_dir, const fs::path& out_dir) {
    auto start = Clock::now();

    // 1. Read manifest.json (simulating metadata processing)
    std::ifstream manifest(pkg_dir / "manifest.json", std::ios::binary);
    if (!manifest) {
        std::cerr << "warning: cannot open manifest: " << (pkg_dir/"manifest.json") << "\n";
        return 0.0;
    }
    // Reading contents into a string to simulate work
    std::string mcontents((std::istreambuf_iterator<char>(manifest)),
                           std::istreambuf_iterator<char>());
    (void)mcontents; // not used further in this simulation

    // 2. Process files in the package's 'files' directory
    fs::path files_dir = pkg_dir / "files";
    if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
        // If 'files' dir doesn't exist, there's nothing to do.
        return 0.0;
    }
    // Create a corresponding output directory for the package
    fs::path out_pkg = out_dir / pkg_dir.filename();
    fs::create_directories(out_pkg);

    // Iterate over each file in the 'files' directory
    for (auto &p : fs::directory_iterator(files_dir)) {
        if (!fs::is_regular_file(p.path())) continue;

        // a. Read file into memory (I/O-bound)
        std::ifstream fin(p.path(), std::ios::binary);
        std::vector<char> buf((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());

        // b. Process file contents (CPU-bound)
        uint64_t cs = checksum_bytes(buf);

        // c. Write file to output directory (I/O-bound)
        fs::path out_file = out_pkg / p.path().filename();
        std::ofstream fout(out_file, std::ios::binary);
        fout.write(buf.data(), buf.size());

        // d. Write metadata (checksum) to a .meta file
        std::ofstream meta(out_pkg / (p.path().filename().string() + ".meta"), std::ios::trunc);
        meta << "checksum:" << cs << "\n";
    }

    // 3. Simulate updating a central database/ledger (atomic append)
    fs::path dbfile = out_dir / "install_db.txt";
    std::ofstream db(dbfile, std::ios::app);
    db << pkg_dir.filename().string() << " installed\n";

    auto end = Clock::now();
    std::chrono::duration<double> dur = end - start;
    return dur.count();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <packages_list.txt> <output_dir>\n";
        return 1;
    }
    fs::path listfile = argv[1];
    fs::path outdir = argv[2];
    fs::create_directories(outdir);

    // Read all package directories from the list file
    std::vector<fs::path> pkg_dirs;
    std::ifstream in(listfile);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        pkg_dirs.push_back(fs::path(line));
    }

    std::cout << "Starting serial processing of " << pkg_dirs.size() << " packages...\n\n";
    auto t0 = Clock::now();

    for (size_t i = 0; i < pkg_dirs.size(); ++i) {
        const auto& pkg = pkg_dirs[i];
        std::cout << "Processing package " << (i + 1) << "/" << pkg_dirs.size()
                  << ": " << pkg.filename().string() << "..." << std::endl;

        double time = process_package_serial(pkg, outdir);

        std::cout << "  -> Done in " << std::fixed << std::setprecision(4) << time << " seconds." << std::endl;
    }

    auto t1 = Clock::now();
    std::chrono::duration<double> dur = t1 - t0;

    std::cout << "\n--------------------------------------------------\n";
    std::cout << "Processed " << pkg_dirs.size() << " packages in "
              << std::fixed << std::setprecision(4) << dur.count()
              << " seconds (serial execution)." << std::endl;
    std::cout << "--------------------------------------------------\n";

    return 0;
}