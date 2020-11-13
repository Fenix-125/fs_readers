// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Remember to include ALL the necessary headers!
#include <iostream>
#include <boost/program_options.hpp>
#include <fstream>
#include <fat_structs.h>
#include <cstring>
#include <boost/format.hpp>

struct fat_date_t {
    explicit fat_date_t(uint16_t date_int) {
        day = date_int & 0b11111u;
        date_int >>= 5u;
        month = date_int & 0b1111u;
        date_int >>= 4u;
        year = date_int;
    }

    unsigned day: 5;
    unsigned month: 4;
    unsigned year: 7;
};

struct fat_time_t {
    explicit fat_time_t(uint16_t time_int) {
        sec = (time_int & 0b0001'1111u) * 2u;
        time_int >>= 5u;
        min = time_int & 0b0011'1111u;
        time_int >>= 6u;
        hour = time_int;
    }

    unsigned sec: 5;
    unsigned min: 6;
    unsigned hour: 5;
};

static inline void print_fat_boot_record_info(fat_boot_t *fs_boot_sector) {
    constexpr char format_str[] = "%-40s %-d\n";
    [[maybe_unused]] uint16_t tmp;
    std::cout << boost::format(format_str) % "Sector size:" % (tmp = fs_boot_sector->bytesPerSector)
              << boost::format(format_str) % "Sectors per cluster:" %
                 static_cast<uint16_t>(fs_boot_sector->sectorsPerCluster)
              << boost::format(format_str) % "FAT copy number:" % static_cast<uint16_t>(fs_boot_sector->fatCount)
              << boost::format(format_str) % "FAT copy size in bytes:" % (fs_boot_sector->sectorsPerFat16 *
                                                                          fs_boot_sector->bytesPerSector)
              << boost::format(format_str) % "FAT copy size in sectors:" % (tmp = fs_boot_sector->sectorsPerFat16)
              << boost::format(format_str) % "Root directory size:" % (tmp = fs_boot_sector->rootDirEntryCount)
              << boost::format(format_str) % "Root directory entry count:" % (tmp = fs_boot_sector->rootDirEntryCount)
              << boost::format(format_str) % "Reserved sectors count:" % (tmp = fs_boot_sector->reservedSectorCount)
              << boost::format("%-40s %-s") % "Check signature:" %
                 ((fs_boot_sector->bootSectorSig0 == 0x55u && fs_boot_sector->bootSectorSig1 == 0XAAu) ? "correct"
                                                                                                       : "incorrect")
              << std::endl;
}

static std::string get_dir_entry_name(const char entry_name[11]) {
    if (entry_name[0] == 0) {
        return "";
    }
    char res[12];
    char *ext = res + 8;
    memcpy(res, entry_name, 11);
    res[11] = '\0';
    if (res[0] == 0x05)
        res[0] = static_cast<char>(0xE5);

    char *tmp_p;
    bool has_ext = true;
    if ((tmp_p = strchr(ext, ' '))) {
        *tmp_p = '\0';
        if (tmp_p == ext)
            has_ext = false;
    }
    if ((tmp_p = strchr(res, ' '))) {
        *tmp_p = '\0';
    }

    std::stringstream s{};
    if (has_ext)
        s << res << '.' << ext;
    else
        s << res;
    return s.str();
}

static inline char get_dir_entry_identifier(const uint16_t i) {
    if (i & 010u)
        return '/';
    else
        return ' ';
}

static inline std::string get_dir_entry_date_time(uint16_t date_int, uint16_t time_int) {
    const fat_date_t date{date_int};
    const fat_time_t time{time_int};
    return (boost::format("%4d-%02d-%02d %02d:%02d:%02d") % (date.year + 1980u) % (date.month + 1u) % (date.day + 1u)
            % (time.hour) % (time.min) % (time.sec)).str();
}

static inline std::string get_dir_entry_status(uint8_t attributes) {
    char res[] = "rhsvda";
    constexpr uint16_t masks[] = {0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u};
    for (uint8_t i = 0; i < 6; ++i)
        if (!(attributes & masks[i]))
            res[i] = '-';
    return res;
}

int main(int argc, char **argv) {
    std::string fs_file;
    {
        namespace po = boost::program_options;

        po::options_description visible("Supported options");
        visible.add_options()
                ("help,h", "Print this help message.");

        po::options_description hidden("Hidden options");
        hidden.add_options()
                ("fs_file", po::value<std::string>(&fs_file), "file with FAT16 file system");

        po::positional_options_description p;
        p.add("fs_file", 1);

        po::options_description all("All options");
        all.add(visible).add(hidden);

        po::variables_map vm;
        try {
            po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
        } catch (const boost::exception &e) {
            std::cerr << "Error: failed to parse arguments!" << std::endl;
            return EXIT_FAILURE;
        }
        po::notify(vm);

        if (vm.count("help")) {
            std::cout
                    << "Usage:\n  fat16_reader [-h|--help] [fs_file]\n\nDescription:\n  Print the main info form file with FAT16.\n\nOptions:"
                    << visible << "\nEntry status description:\n"
                                  "  r\tRead Only\n"
                                  "  h\tHidden\n"
                                  "  s\tSystem\n"
                                  "  v\tVolume Label\n"
                                  "  d\tDirectory\n"
                                  "  a\tArchive" << std::endl;
            return EXIT_SUCCESS;
        }
        if (vm.count("fs_file") == 0) {
            std::cerr << "Error: not fs_file file supplied. Go to --help for details!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::ifstream fs_stream(fs_file, std::ios_base::binary);
    std::vector<unsigned char> fs_bin(std::istreambuf_iterator<char>(fs_stream), {});
    auto *data_p = static_cast<unsigned char *>(fs_bin.data());

    auto *fs_boot_sector = reinterpret_cast<fat_boot_t *>(data_p);
    std::cout << "Read '" << fs_file << "' as FAT16 file system:" << std::endl;

    print_fat_boot_record_info(fs_boot_sector);

    auto *dir_entry = reinterpret_cast<const dir_t *>(data_p + 512u +
                                                      fs_boot_sector->fatCount * fs_boot_sector->sectorsPerFat16 *
                                                      fs_boot_sector->bytesPerSector);
    std::cout << "\nRoot dir entries info:" << std::endl;
    std::cout << boost::format("%=6s\t%=10s\t%=19s\t%7d\t%5d\t%s") % "status" % "size" % "last modified" % "cluster" %
                 "block" % "file name"
              << std::endl;
    for (int i = 0; i < fs_boot_sector->rootDirEntryCount; ++i) {
        if (dir_entry->name[0] != 0xE5 && dir_entry->name[0] != 0x00)
            std::cout
                    << boost::format("%6s\t%10d\t%19s\t%7d\t%5d\t%s%c") % get_dir_entry_status(dir_entry->attributes) %
                       dir_entry->fileSize %
                       get_dir_entry_date_time(dir_entry->lastWriteDate, dir_entry->lastWriteTime) %
                       dir_entry->firstClusterLow % (dir_entry->firstClusterLow * fs_boot_sector->sectorsPerCluster) %
                       get_dir_entry_name(reinterpret_cast<const char *>(dir_entry->name)) %
                       get_dir_entry_identifier(dir_entry->attributes) << std::endl;
        ++dir_entry;
    }
    return EXIT_SUCCESS;
}
