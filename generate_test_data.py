import os
import random

def generate_test_data(num_packages, num_files_per_package):
    if not os.path.exists("pkgs"):
        os.makedirs("pkgs")

    with open("packages.txt", "w") as f:
        for i in range(1, num_packages + 1):
            pkg_name = f"pkg{i:03d}"
            pkg_dir = os.path.join("pkgs", pkg_name)
            files_dir = os.path.join(pkg_dir, "files")
            os.makedirs(files_dir, exist_ok=True)

            with open(os.path.join(pkg_dir, "manifest.json"), "w") as manifest:
                manifest.write(f'{{"name":"{pkg_name}","version":"1.0.0"}}\n')

            for j in range(1, num_files_per_package + 1):
                file_name = f"f{j}.bin"
                file_path = os.path.join(files_dir, file_name)
                with open(file_path, "wb") as bin_file:
                    size = 1024 + random.randint(0, 4096)
                    bin_file.write(os.urandom(size))
            
            f.write(f"{pkg_dir}\n")

if __name__ == "__main__":
    generate_test_data(100, 20)
