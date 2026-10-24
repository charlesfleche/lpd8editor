// Runs a fuzzer once on each file given on the command line, or found in a
// folder given on the command line: the seeds of a corpus, or the input
// that made a fuzzer fail. Used instead of libFuzzer's main() when not
// fuzzing, so that every build and every compiler runs the corpus as tests.

#include "fuzzer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <vector>

namespace {

bool run(const std::filesystem::path &file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot read " << file << '\n';
        return false;
    }
    const std::vector<char> bytes{std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>()};
    // A copy of exactly the file's size, as libFuzzer would give it.
    const std::vector<std::uint8_t> data(bytes.begin(), bytes.end());
    LLVMFuzzerTestOneInput(data.data(), data.size());
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    LLVMFuzzerInitialize(&argc, &argv);

    int count = 0;
    const std::span<char *> args(argv, static_cast<std::size_t>(argc));
    for (const char *arg : args.subspan(1)) {
        const std::filesystem::path path(arg);
        std::vector<std::filesystem::path> files;
        if (std::filesystem::is_directory(path)) {
            for (const auto &entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }
        } else {
            files.push_back(path);
        }
        for (const auto &file : files) {
            if (!run(file)) {
                return EXIT_FAILURE;
            }
            ++count;
        }
    }
    std::cout << "Ran " << count << " inputs\n";
    // An empty corpus means a wrong path: fail rather than test nothing.
    return count > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
