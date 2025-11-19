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

void info(void) {
    std::cerr
    << "          ***********     \n"
    << "        ************      \n"
    << "      ************        \n"
    << "    ************  **      \n"
    << "  ************  ******    \n"
    << "************  **********  \n"
    << "**********    ************\n"
    << "************    **********\n"
    << "  **********  ************\n"
    << "    ******  ************  \n"
    << "      **  ************    \n"
    << "        ************      \n"
    << "      ************        \n"
    << "    ************          \n\n"
    << "Copyright (C) 2024-" << YEAR << " Insoft.\n"
    << "Insoft " << NAME << "\n\n";
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

std::filesystem::path expand_tilde(const std::filesystem::path& path) {
    if (!path.empty() && path.string().starts_with("~")) {
#ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
#else
        const char* home = std::getenv("HOME");
#endif
        
        if (home) {
            return std::string(home) + path.string().substr(1);  // Replace '~' with $HOME
        }
    }
    return path;  // return as-is if no tilde or no HOME
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
                outpath = fs::path(argv[n]);
                outpath = expand_tilde(outpath);
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
        inpath = fs::path(argv[n]);
        inpath = expand_tilde(inpath);
        if (inpath.parent_path().empty()) {
            inpath = fs::path("./") / inpath;
        }
    }
    
    if (outpath != "/dev/stdout") info();
    
    
    /*
     If the input file does not have an extension, default .hpprgm is assumed and applied.
     */
    if (inpath.extension().empty()) {
        inpath.replace_extension("hpprgm");
    }
    
    /*
     If no output file is specified, the program will use the input file’s name
     (excluding its extension) as the output file name.
     */
    if (outpath.empty() || outpath == inpath) {
        outpath = inpath;
        outpath.replace_extension("");
    }
    
    /*
     If the output file does not have an extension, a default .prgm or .hpprgm is applied.
     */
    if (outpath.extension().empty()) {
        if (fs::is_directory(outpath)) {
            outpath = outpath / inpath.stem();
        }
        outpath.replace_extension((inpath.extension() == ".hpprgm" ? "prgm" : "hpprgm"));
    }

    /*
     We need to ensure that the specified output filename includes a path.
     If no path is provided, we prepend the path from the input file.
     */
    if (outpath.parent_path().empty()) {
        outpath = inpath.parent_path() / outpath;
    }
    
    /*
     The output file must differ from the input file. If they are the same, the
     process will be halted and an error message returned to the user.
     */
    if (inpath == outpath) {
        std::cerr << "❌ Error: The output file must differ from the input file. Please specify a different output file name.\n";
        return 0;
    }
    
    if (!fs::exists(inpath)) {
        std::cerr << "❌ Error: The specified input ‘" << inpath.filename() << "‘ file is invalid or not supported. Please ensure the file exists and has a valid format.\n";
    }
    
    std::ifstream infile;
    
    infile.open(inpath, std::ios::in | std::ios::binary);
    if(!infile.is_open()) {
        std::cerr << "❌ Error: The specified input ‘" << inpath.filename() << "‘ file not found.\n";
        return 0;
    }
    
    if (std::filesystem::path(outpath).extension() == ".prgm") {
        std::wstring wstr = hpprgm::load(inpath);
        utf::save(outpath, wstr);
    }
    
    if (outpath.extension() == ".hpprgm") {
        std::wstring wstr = utf::load(inpath);
        std::string str = utf::utf8(wstr);
        hpprgm::save(outpath, str);
    }
    
    if (std::filesystem::exists(outpath)) {
        std::cerr << "✅ File " << outpath.filename() << " succefuly created.\n";
    }
    
    return 0;
}

