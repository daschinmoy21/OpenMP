
// bun_sim_serial.cpp
// Serial simulation of package installation (C++).
// Compile: g++ -O2 -std=c++17 bun_sim_serial.cpp -o bun_serial
// Usage: ./bun_serial <packages_list.txt> <output_dir>
// packages_list.txt: each line: <pkg_dir> (pkg_dir contains manifest.json and files/ subdir)
// Example: ./bun_serial packages.txt out_serial

#include <bits/stdc++.h>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

uint64_t checksum_bytes(const std::vector<char>& data) {
    // simple CPU-bound checksum to simulate processing
    uint64_t h = 1469598103934665603ULL;
    for (char c : data) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

void process_package_serial(const fs::path& pkg_dir, const fs::path& out_dir) {
    // 1. read manifest.json
    std::ifstream manifest(pkg_dir / "manifest.json", std::ios::binary);
    if (!manifest) {
        std::cerr << "warning: cannot open manifest: " << (pkg_dir/"manifest.json") << "\n";
        return;
    }
    std::string mcontents((std::istreambuf_iterator<char>(manifest)),
                           std::istreambuf_iterator<char>());
    (void)mcontents; // not used further in this simulation

    // 2. list files in pkg_dir/files/
    fs::path files_dir = pkg_dir / "files";
    if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) {
        // nothing to do
        return;
    }
    // create output package dir
    fs::path out_pkg = out_dir / pkg_dir.filename();
    fs::create_directories(out_pkg);

    for (auto &p : fs::directory_iterator(files_dir)) {
        if (!fs::is_regular_file(p.path())) continue;
        // read file
        std::ifstream fin(p.path(), std::ios::binary);
        std::vector<char> buf((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());

        // CPU work: checksum (simulates parsing/processing)
        uint64_t cs = checksum_bytes(buf);

        // write file to output (simulate install copy)
        fs::path out_file = out_pkg / p.path().filename();
        std::ofstream fout(out_file, std::ios::binary);
        fout.write(buf.data(), buf.size());

        // write a small metadata file with checksum
        std::ofstream meta(out_pkg / (p.path().filename().string() + ".meta"), std::ios::trunc);
        meta << "checksum:" << cs << "\n";
    }

    // simulate DB update (atomic append)
    fs::path dbfile = out_dir / "install_db.txt";
    std::ofstream db(dbfile, std::ios::app);
    db << pkg_dir.filename().string() << " installed\n";
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

    auto t0 = Clock::now();
    for (auto &pkg : pkg_dirs) {
        process_package_serial(pkg, outdir);
    }
    auto t1 = Clock::now();
    std::chrono::duration<double> dur = t1 - t0;
    std::cout << "Processed " << pkg_dirs.size() << " packages in " << dur.count() << " seconds (serial)\n";
    return 0;
}
