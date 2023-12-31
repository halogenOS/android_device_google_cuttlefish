/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <string>
#include <vector>

#include "common/libs/utils/result.h"

namespace cuttlefish {
bool FileExists(const std::string& path, bool follow_symlinks = true);
bool FileHasContent(const std::string& path);
Result<std::vector<std::string>> DirectoryContents(const std::string& path);
bool DirectoryExists(const std::string& path, bool follow_symlinks = true);
Result<void> EnsureDirectoryExists(const std::string& directory_path,
                                   const mode_t mode = S_IRWXU | S_IRWXG |
                                                       S_IROTH | S_IXOTH,
                                   const std::string& group_name = "");
Result<void> ChangeGroup(const std::string& path,
                         const std::string& group_name);
bool CanAccess(const std::string& path, const int mode);
bool IsDirectoryEmpty(const std::string& path);
bool RecursivelyRemoveDirectory(const std::string& path);
bool Copy(const std::string& from, const std::string& to);
off_t FileSize(const std::string& path);
bool RemoveFile(const std::string& file);
Result<std::string> RenameFile(const std::string& current_filepath,
                               const std::string& target_filepath);
std::string ReadFile(const std::string& file);
bool MakeFileExecutable(const std::string& path);
std::chrono::system_clock::time_point FileModificationTime(const std::string& path);
std::string cpp_dirname(const std::string& str);
std::string cpp_basename(const std::string& str);
// Whether a file exists and is a unix socket
bool FileIsSocket(const std::string& path);
// Get disk usage of a path. If this path is a directory, disk usage will
// account for all files under this folder(recursively).
int GetDiskUsage(const std::string& path);

// The returned value may contain .. or . if these are present in the path
// argument.
// path must not contain ~
std::string AbsolutePath(const std::string& path);

std::string CurrentDirectory();

struct FileSizes {
  off_t sparse_size;
  off_t disk_size;
};
FileSizes SparseFileSizes(const std::string& path);

// Find file with name |target_name| under directory |path|, return path to
// found file(if any)
std::string FindFile(const std::string& path, const std::string& target_name);

Result<void> WalkDirectory(
    const std::string& dir,
    const std::function<bool(const std::string&)>& callback);

Result<void> WaitForFile(const std::string& path, int timeoutSec);
Result<void> WaitForUnixSocket(const std::string& path, int timeoutSec);

}  // namespace cuttlefish
