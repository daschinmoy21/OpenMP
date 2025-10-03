
// bun_sim_parallel.cpp
// Parallel simulation of package installation using OpenMP.
// Compile: g++ -O2 -fopenmp -std=c++17 bun_sim_parallel.cpp -o bun_parallel
// Usage: ./bun_parallel <packages_list.txt> <output_dir>

#include <bits/stdc++.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <omp.h>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

uint64_t checksum_bytes(const std::vector<char>& data) {
    uint64_t h = 1469598103934665603ULL;
    for (char c : data) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

void process_package(const fs::path& pkg_dir, const fs::path& out_dir) {
    // read manifest
    std::ifstream manifest(pkg_dir / "manifest.json", std::ios::binary);
    if (!manifest) return;
    std::string mcontents((std::istreambuf_iterator<char>(manifest)),
                           std::istreambuf_iterator<char>());

    fs::path files_dir = pkg_dir / "files";
    if (!fs::exists(files_dir) || !fs::is_directory(files_dir)) return;

    fs::path out_pkg = out_dir / pkg_dir.filename();
    fs::create_directories(out_pkg);

    for (auto &p : fs::directory_iterator(files_dir)) {
        if (!fs::is_regular_file(p.path())) continue;
        std::ifstream fin(p.path(), std::ios::binary);
        std::vector<char> buf((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());

        uint64_t cs = checksum_bytes(buf);

        fs::path out_file = out_pkg / p.path().filename();
        std::ofstream fout(out_file, std::ios::binary);
        fout.write(buf.data(), buf.size());
        std::ofstream meta(out_pkg / (p.path().filename().string() + ".meta"), std::ios::trunc);
        meta << "checksum:" << cs << "\n";
    }

    // update DB (serialized)
    fs::path dbfile = out_dir / "install_db.txt";
    // Use of append with lock via OpenMP critical:
    #pragma omp critical
    {
        std::ofstream db(dbfile, std::ios::app);
        db << pkg_dir.filename().string() << " installed\n";
    }
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

    // OpenMP parallel for
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < (int)pkg_dirs.size(); ++i) {
        process_package(pkg_dirs[i], outdir);
    }

    auto t1 = Clock::now();
    std::chrono::duration<double> dur = t1 - t0;
    int threads = omp_get_max_threads();
    std::cout << "Processed " << pkg_dirs.size() << " packages in " << dur.count()
              << " seconds (parallel, threads=" << threads << ")\n";
    return 0;
}
