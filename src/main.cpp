#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

// ---------------- CONFIGURATION ----------------
const std::string USERNAME_STRING = std::getenv("USER");
const fs::path DOWNLOADS_FOLDER_PATH =
    "/home/" + USERNAME_STRING + "/Downloads/";
const fs::path CONFIG_PATH =
    "/home/" + USERNAME_STRING + "/.config/eyeball/paths.config";

// ---------------- GLOBALS ----------------
bool running = true;

// ---------------- SIGNAL HANDLER ----------------
void handleSignal(int signum) {
  if (signum == SIGTERM)
    running = false;
}

// ---------------- DAEMONIZE ----------------
void daemonize() {
  pid_t pid = fork();
  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS);

  if (setsid() < 0)
    exit(EXIT_FAILURE);
  umask(0);

  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
  freopen("/dev/null", "w", stderr);
}

// ---------------- CONFIG LOADER ----------------
std::map<std::string, fs::path> load_config(const fs::path &configPath) {
  std::map<std::string, fs::path> rules;
  std::ifstream infile(configPath);
  if (!infile.is_open()) {
    std::cerr << "Could not open config file: " << configPath << "\n";
    return rules;
  }

  std::string line;
  while (std::getline(infile, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream iss(line);
    std::string ext, path;
    if (std::getline(iss, ext, '=') && std::getline(iss, path)) {
      if (!ext.empty() && ext[0] != '.')
        ext = "." + ext;
      rules[ext] = fs::path(path);
    }
  }
  return rules;
}

// ---------------- FILE MOVE ----------------
void moveFile(const fs::path &source, const fs::path &dest) {
  try {
    if (!fs::exists(source) || !fs::is_regular_file(source))
      return;
    fs::create_directories(dest.parent_path());

    try {
      fs::rename(source, dest);
    } catch (fs::filesystem_error &) {
      fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
      fs::remove(source);
    }
    std::cout << "Moved: " << source << " -> " << dest << "\n";
  } catch (fs::filesystem_error &e) {
    std::cerr << "Error moving file: " << e.what() << "\n";
  }
}

void sortExisting(const std::map<std::string, fs::path> &filePaths) {
  for (const auto &entry : fs::directory_iterator(DOWNLOADS_FOLDER_PATH)) {
    if (!entry.is_regular_file())
      continue;

    auto filePath = entry.path();
    auto ext = filePath.extension().string();

    if (ext == ".part" || ext == ".crdownload")
      continue;

    if (filePaths.count(ext)) {
      moveFile(filePath, filePaths.at(ext) / filePath.filename());
    }
  }
}

// ---------------- FILE WATCH LOOP ----------------
void watchLoop() {
  std::map<std::string, fs::path> filePaths = load_config(CONFIG_PATH);

  int fd = inotify_init1(IN_NONBLOCK);
  if (fd < 0) {
    perror("inotify_init");
    return;
  }

  int wd = inotify_add_watch(fd, DOWNLOADS_FOLDER_PATH.c_str(),
                             IN_CREATE | IN_MOVED_TO);
  if (wd < 0) {
    perror("inotify_add_watch");
    return;
  }

  const size_t event_size = sizeof(struct inotify_event);
  const size_t buf_len = 1024 * (event_size + 16);
  char buffer[buf_len];

  // sort the files in downloads first
  sortExisting(filePaths);

  while (running) {
    int length = read(fd, buffer, buf_len);
    if (length < 0) {
      if (errno == EAGAIN) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        continue;
      } else {
        perror("read");
        break;
      }
    }

    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        fs::path filePath = DOWNLOADS_FOLDER_PATH / event->name;
        if (filePath.extension() != ".part" &&
            filePath.extension() != ".crdownload") {
          auto ext = filePath.extension().string();
          if (filePaths.count(ext)) {
            moveFile(filePath, filePaths[ext] / filePath.filename());
          }
        }
      }
      i += event_size + event->len;
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);
}

// ---------------- HELPER FUNCTIONS ----------------
bool isEyeballRunning() {
  pid_t self = getpid(); // current process ID
  DIR *proc = opendir("/proc");
  if (!proc)
    return false;

  struct dirent *entry;
  while ((entry = readdir(proc)) != nullptr) {
    if (entry->d_type != DT_DIR)
      continue;

    // skip non-numeric directories
    bool numeric = true;
    for (const char *p = entry->d_name; *p; ++p) {
      if (!std::isdigit(*p)) {
        numeric = false;
        break;
      }
    }
    if (!numeric)
      continue;

    pid_t pid = std::stoi(entry->d_name);
    if (pid == self)
      continue; // skip self

    std::string commPath = std::string("/proc/") + entry->d_name + "/comm";
    std::ifstream commFile(commPath);
    if (!commFile.is_open())
      continue;

    std::string name;
    std::getline(commFile, name);
    if (name == "eyeball") {
      closedir(proc);
      return true;
    }
  }
  closedir(proc);
  return false;
}

void stopAllEyeballProcesses() {
  DIR *proc = opendir("/proc");
  if (!proc)
    return;

  struct dirent *entry;
  while ((entry = readdir(proc)) != nullptr) {
    if (entry->d_type != DT_DIR)
      continue;

    if (!std::all_of(entry->d_name, entry->d_name + std::strlen(entry->d_name),
                     [](char c) { return std::isdigit(c); }))
      continue;

    std::string commPath = std::string("/proc/") + entry->d_name + "/comm";
    std::ifstream commFile(commPath);
    if (!commFile.is_open())
      continue;

    std::string name;
    std::getline(commFile, name);
    if (name == "eyeball") {
      pid_t pid = std::stoi(entry->d_name);
      kill(pid, SIGTERM);
    }
  }
  closedir(proc);
}

// ---------------- MAIN ----------------
int main(int argc, char *argv[]) {
  std::signal(SIGTERM, handleSignal);

  if (argc < 2) {
    std::cout << "Usage: eyeball <on|off|status>\n";
    return 0;
  }

  std::string cmd = argv[1];

  if (cmd == "on") {
    if (isEyeballRunning()) {
      std::cout << "eyeball is already running\n";
      return 0;
    }

    daemonize();
    watchLoop();

  } else if (cmd == "off") {
    stopAllEyeballProcesses();
    std::cout << "eyeball stopped\n";

  } else if (cmd == "status") {
    if (isEyeballRunning()) {
      std::cout << "eyeball is watching\n";
    } else {
      std::cout << "eyeball is not watching\n";
    }

  } else {
    std::cout << "Unknown command: " << cmd << "\n";
    std::cout << "Usage: eyeball <on|off|status>\n";
  }

  return 0;
}
