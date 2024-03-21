#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>

#include "commutils.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

class FileOperate {
 public:
  /**
   * @brief Checks if a file exists.
   *
   * This function checks if the specified file exists in the file system.
   *
   * @param f The path of the file to check.
   * @return true if the file exists, false otherwise.
   */
  bool IsExist(const std::string& f) {
    struct stat buf;
    if (0 != stat(f.c_str(), &buf)) {
      LOG(WARNING) << fmt::format("file {} is NOT Existed", f);
      return false;
    }
    LOG(DEBUG) << "file " << f << " is Existed";
    return true;
  }

  /**
   * @brief Creates an empty file with the specified name.
   *
   * This function creates a new file with the given name and initializes it as
   * an empty file.
   *
   * @param f The name of the file to be created.
   * @return 0 if the file is created successfully, -1 otherwise.
   */
  int CreateEmpty(const std::string& f) {
    if (true == IsExist(f)) {
      LOG(ERROR) << "file " << f << " is already existed!";
      return -1;
    }
    std::ofstream _of(f, std::ios::out);
    if (!_of.is_open()) {
      LOG(ERROR) << f << " create failed!";
      return -1;
    }
    LOG(INFO) << f << " created Successfully~";
    _of.close();
    return 0;
  }

  /**
   * @brief Searches for a file recursively in a directory.
   *
   * This function searches for a file with the given name (`f`) in the
   * specified directory (`d`) and its subdirectories. The search can be limited
   * to a specific depth (`depth`). By default, the search is limited to a depth
   * of 1. The function also supports searching for files that match a specific
   * pattern (`pattern`). By default, the pattern is set to "regex". Or
   * "keyword".
   * 
   * @attention If your GNU version is lower than 4.9, you can't use the regex 
   *            pattern, it will always return false.
   * 
   * @param fp A reference to a string that will store the full path of the
   * found file (if found).
   * @param f The name of the file to search for.
   * @param d The directory to start the search from.
   * @param depth The maximum depth of the search (optional, default is 1).
   * @param pattern The pattern to match against the file names (optional,
   * default is "keyword").
   * @return `true` if the file is found, `false` otherwise.
   */
  bool SearchRecursively(std::string& fp, const std::string& f,
                             const std::string& d, int depth = 1,
                             const std::string& pattern = "keyword") {
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9)
    if (pattern == "regex") return false;
#endif
    if (depth <= 0) return false;
    struct dirent* end = nullptr;
    std::string curDir = d[d.length() - 1] == '/' ? d : d + '/';
    std::string tmpFile;
    DIR* fDir = opendir(curDir.c_str());
    if (nullptr == fDir) {
      LOG(ERROR) << "Open directory[" << curDir << "] failed!";
      return false;
    }
    while ((end = readdir(fDir)) != nullptr) {
      if (std::string(end->d_name) == "." || std::string(end->d_name) == "..") {
        continue;
      }
      if (end->d_type == DT_DIR) {
        std::string nextDir = curDir + end->d_name;
        if (true == SearchRecursively(fp, f, nextDir, depth - 1, pattern)) {
          closedir(fDir);
          return true;
        }
        continue;
      }
      tmpFile = end->d_name;
      if (pattern == "keyword") {
        if (tmpFile.npos != tmpFile.find(f)) {
          fp = curDir + tmpFile;
          LOG(INFO) << "Search file Successfully~ [" << fp << ']';
          closedir(fDir);
          return true;
        }
      } else if (pattern == "regex") {
        std::regex e(f);
        if (std::regex_search(tmpFile, e)) {
          fp = curDir + tmpFile;
          LOG(INFO) << "Search file Successfully~ [" << fp << ']';
          closedir(fDir);
          return true;
        }
      }
    }
    LOG(DEBUG) << "File [" << f << "] can't be found under the dir[" << d
               << ']';
    closedir(fDir);
    return false;
  }

  /**
   * @brief Reads the contents of a file into a JSON object.
   *
   * @param f The path to the file to be read.
   * @param buf The JSON object to store the file contents.
   * @param m The open mode for the file (optional, default is std::ios::in).
   * @return The number of characters read from the file, or -1 if an error
   * occurred.
   */
  int ReadFrom(const std::string& f, json& buf,
                   std::ios::openmode m = std::ios::in) {
    std::ifstream _if(f, m);
    if (!_if.is_open()) {
      LOG(ERROR) << "File [" << f << "] open failed!!!";
      return -1;
    }
    _if >> buf;
    _if.seekg(0, std::ios::end);
    std::streampos rz = _if.tellg();
    _if.close();
    return static_cast<long>(rz);
  }

  /**
   * @brief Reads the contents of a file into a string buffer.
   *
   * @param f The path of the file to read from.
   * @param buf The string buffer to store the file contents.
   * @param m The open mode for the file (default is std::ios::in).
   * @return The number of characters read from the file, or -1 if an error
   * occurred.
   */
  int ReadFrom(const std::string& f, std::string& buf,
                   std::ios::openmode m = std::ios::in) {
    std::ifstream _if(f, m);
    if (!_if.is_open()) {
      LOG(ERROR) << "File [" << f << "] open failed!!!";
      return -1;
    }

    _if.seekg(0, std::ios::end);
    buf.resize(_if.tellg());
    _if.seekg(0, std::ios::beg);
    _if.read(&buf[0], buf.size());

    _if.close();
    return buf.length();
  }

  /**
   * @brief Get the name of a file without the file suffix.
   *
   * @param f The file name with the suffix.
   * @return The file name without the suffix.
   */
  std::string GetNameWithoutSuffix(const std::string& f) {
    std::string name;
    std::string head;
    size_t pos = f.rfind('/') == f.npos ? 0 : f.rfind('/') + 1;
    name = f.substr(pos, f.length() - pos);
    if ('.' == name[0]) {
      name = name.substr(1, name.length() - 1);
      head = ".";
    }
    pos = name.find('.') == f.npos ? name.length() : name.find('.');
    name = name.substr(0, pos);
    name = head + name;
    return name;
  }

  /**
   * @brief Get the suffix of a file.
   *
   * This function takes a file path as input and returns the suffix of the
   * file.
   *
   * @param f The file path.
   * @return The suffix of the file.
   */
  std::string GetSuffix(const std::string& f) {
    std::string suffix;
    size_t pos = f.rfind('/') == f.npos ? 0 : f.rfind('/') + 1;
    suffix = f.substr(pos, f.length() - pos);
    // 当文件名以 '.' 开头时，排除该 '.'
    if ('.' == suffix[0]) suffix = suffix.substr(1, suffix.length() - 1);
    pos = suffix.find('.') == suffix.npos ? suffix.length()
                                          : suffix.find('.') + 1;
    suffix = suffix.substr(pos, suffix.length() - pos);
    return suffix;
  }

  /**
   * @brief Returns a list of files in a directory that match the specified
   *        type and keyword.
   *
   * This function searches for files in the specified directory that have the
   * specified type and optionally contain the specified keyword. The matching
   * sssfiles are returned as a vector of strings.
   *
   * @param dir The directory to search in.
   * @param type The file type to match (e.g., "dir", "file").
   * @param keyword The keyword to search for in the file names (optional).
   * @return A vector of strings containing the names of the matching files.
   */
  std::vector<std::string> ListInDir(const std::string& dir,
                                         const std::string& type,
                                         const std::string& keyword = "") {
    std::vector<std::string> fl;
    int _d_type =
        type == "dir" ? static_cast<int>(DT_DIR) : static_cast<int>(~DT_DIR);
    DIR* fDir = opendir(dir.c_str());
    if (nullptr == fDir) {
      LOG(ERROR) << "Open directory[" << dir << "] failed!";
      return fl;
    }
    struct dirent* end = nullptr;
    while ((end = readdir(fDir)) != nullptr) {
      if (end->d_type & _d_type) {
        std::string fname = std::string(end->d_name);
        if (std::regex_search(fname, std::regex(keyword)) ||
            fname.npos != fname.find(keyword) || keyword.length() <= 0) {
          fl.push_back(dir[dir.length() - 1] == '/' ? dir + end->d_name
                                                    : dir + "/" + end->d_name);
        }
      }
    }
#ifndef NDEBUG
    LOG(INFO) << "list path [" << dir << "], type [" << type << "], key word ["
              << keyword << ']';
    std::for_each(fl.begin(), fl.end(),
                  [](const std::string& it) { LOG(INFO) << it; });
#endif
    closedir(fDir);
    return fl;
  }

  /**
   * @brief Creates directories along the specified path.
   *
   * This function creates directories along the specified path. If any of the
   * directories in the path do not exist, they will be created.
   *
   * @param p The path for which directories need to be created.
   * @param mode The permissions to be set for the created directories.
   *             Default value is S_IRWXU | S_IRWXG | S_IRWXO.
   * @return Returns 0 on success, or -1 on failure.
   */
  int MkdirsByPath(const std::string& p,
                       mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO) {
    if (access(p.c_str(), F_OK) != 0) {
      int ret = MkdirsByPath(p.substr(0, p.rfind('/')), mode);
      if (0 == ret) {
        ret = mkdir(p.c_str(), mode);
        if (0 != ret) LOG(ERROR) << "mkdir [" << p << "] Failed!";
      }
      return ret;
    }
    return 0;
  }

  /**
   * @brief Writes the content of a buffer to a file.
   *
   * @param f The path to the file.
   * @param buf The buffer containing the data to be written.
   * @param m The open mode for the file (optional, defaults to std::ios::out).
   * @return The number of characters written to the file.
   */
  template <typename T>
  int WriteTo(const std::string& f, const T& buf,
                  std::ios::openmode m = std::ios::out) {
    std::ofstream _of(f);
    if (!_of.is_open()) {
      LOG(ERROR) << "File[" << f << "] open failed!!!";
      return -1;
    }
    _of << buf;
    _of.seekp(0, std::ios::end);
    std::streampos wz = _of.tellp();
    _of.close();
    return static_cast<long>(wz);
  }
};