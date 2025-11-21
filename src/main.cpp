// The MIT License (MIT)
//
// Copyright (c) 2024-2025 Insoft.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include "hpprgm.hpp"
#include "utf.hpp"

static bool verbose = false;

// TODO: Impliment "Indices of glyphs"

#include "../version_code.h"
#define NAME "HP Prime Program Tool"
#define COMMAND_NAME "hpprgm"

// MARK: - Command Line
void version(void) {
    using namespace std;
    std::cerr
    << "Copyright (C) 2024-" << YEAR << " Insoft.\n"
    << "Insoft "<< NAME << " version, " << VERSION_NUMBER << " (BUILD " << BUNDLE_VERSION << ")\n"
    << "Built on: " << DATE << "\n"
    << "Licence: MIT License\n\n"
    << "For more information, visit: http://www.insoft.uk\n";
}

void error(void) {
    std::cerr << COMMAND_NAME << ": try '" << COMMAND_NAME << " --help' for more information\n";
    exit(0);
}

void help(void) {
    std::cerr
    << "Copyright (C) 2024-" << YEAR << " Insoft.\n"
    << "Insoft "<< NAME << " version, " << VERSION_NUMBER << " (BUILD " << BUNDLE_VERSION << ")\n"
    << "\n"
    << "Usage: " << COMMAND_NAME << " <input-file> [-o <output-file>] [-v flags]"
    << ""
    << "Options:"
    << "  -o <output-file>   Specify the filename for generated .hpprgm or .prgm file."
    << "  -v                 Enable verbose output for detailed processing information."
    << ""
    << "Verbose Flags:"
    << "  s                  Size of extracted PPL code in bytes."
    << ""
    << "Additional Commands:"
    << "  " << COMMAND_NAME << " {--version | --help}"
    << "    --version        Display version information."
    << "    --help           Show this help message.";
}

// MARK: - Extensions

namespace fs = std::filesystem;

namespace std::filesystem {
    std::filesystem::path expand_tilde(const std::filesystem::path& path) {
        if (!path.empty() && path.string().starts_with("~")) {
#ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
#else
            const char* home = std::getenv("HOME");
#endif
            
            if (home) {
                return std::filesystem::path(std::string(home) + path.string().substr(1));  // Replace '~' with $HOME
            }
        }
        return path;  // return as-is if no tilde or no HOME
    }
}


#if __cplusplus >= 202302L
    #include <bit>
    using std::byteswap;
#elif __cplusplus >= 201103L
    #include <cstdint>
    namespace std {
        template <typename T>
        T byteswap(T u)
        {
            
            static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");
            
            union
            {
                T u;
                unsigned char u8[sizeof(T)];
            } source, dest;
            
            source.u = u;
            
            for (size_t k = 0; k < sizeof(T); k++)
                dest.u8[k] = source.u8[sizeof(T) - k - 1];
            
            return dest.u;
        }
    }
#else
    #error "C++11 or newer is required"
#endif

fs::path resolveAndValidateInputFile(const char *input_file) {
    fs::path path;
    
    path = input_file;
    if (path == "/dev/stdin") return path;
    
    path = fs::expand_tilde(path);
    if (path.parent_path().empty()) path = fs::path("./") / path;
    
    // • Applies a default extension
    if (path.extension().empty()) path.replace_extension("hpprgm");
    
    if (!fs::exists(path)) {
        std::cerr << "❓File " << path.filename() << " not found at " << path.parent_path() << " location.\n";
        exit(0);
    }
    
    return path;
}

fs::path resolveOutputFile(const char *output_file) {
    fs::path path;
    
    path = output_file;
    if (path == "/dev/stdout") return path;
    
    path = fs::expand_tilde(path);
    
    return path;
}

fs::path resolveOutputPath(const fs::path& inpath, const fs::path& outpath) {
    fs::path path = outpath;
    
    if (path == "/dev/stdout") return path;
    
    if (path.empty()) path = inpath;
    if (fs::is_directory(path)) path = path / inpath.filename();
    path.replace_extension((inpath.extension() == ".hpprgm" ? "prgm" : "hpprgm"));
    if (path.parent_path().empty()) path = inpath.parent_path() / path;
    
    return path;
}

// MARK: - Main

int main(int argc, const char **argv)
{
    namespace fs = std::filesystem;
    
    fs::path inpath, outpath;
    
    if (argc == 1) {
        error();
        return 0;
    }
    
    std::string args(argv[0]);

    for( int n = 1; n < argc; n++ ) {
        if (*argv[n] == '-') {
            std::string args(argv[n]);
            
            if (args == "-o") {
                if (++n > argc) error();
                outpath = resolveOutputFile(argv[n]);
                continue;
            }

            if (args == "--help") {
                help();
                return 0;
            }
            
            if (args == "--version") {
                version();
                return 0;
            }
            
            if (args == "-v") {
                if (++n > argc) error();
                args = argv[n];
                verbose = true;
                continue;
            }
            
            error();
            return 0;
        }
        
        inpath = resolveAndValidateInputFile(argv[n]);
    }
    
    outpath = resolveOutputPath(inpath, outpath);
    
    std::ifstream infile;
    
    infile.open(inpath, std::ios::in | std::ios::binary);

    if (!infile.is_open()) {
        std::cerr << "❓File " << inpath.filename() << " not found at " << inpath.parent_path() << " location.\n";
        exit(0);
    }
    
    std::wstring wstr;
    
    if (inpath.extension() == ".hpprgm" || inpath.extension() == ".hpappprgm") {
        wstr = hpprgm::load(inpath);
    } else {
        wstr = utf::load(inpath, utf::BOMle);
    }
    
    if (outpath == "/dev/stdout") {
        std::cout << utf::utf8(wstr);
    } else {
        if (outpath.extension() == ".hpprgm") {
            hpprgm::save(outpath, utf::utf8(wstr));
        } else {
            utf::save(outpath, wstr);
        }
    }
    
    if (!std::filesystem::exists(outpath)) {
        std::cerr << "❌ Unable to create file " << outpath.filename() << ".\n";
        return 0;
    }

    std::cerr << "✅ File " << outpath.filename() << " succefuly created.\n";
    
    return 0;
}

