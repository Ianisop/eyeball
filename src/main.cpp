#include <filesystem>
#include <iostream>
#include <limits.h>
#include <map>
#include <string>
#include <sys/inotify.h>
#include <unistd.h>

const char *username = std::getenv("USER");
const std::string USERNAME_STRING(username);
const std::string DOWNLOADS_FOLDER_PATH =
    "/home/" + USERNAME_STRING + "/Downloads/";

int fd, wd;

bool init_file_watcher() {

  fd = inotify_init();

  if (fd < 0) {
    perror("inotify_init");
    return false;
  }

  wd = inotify_add_watch(fd, DOWNLOADS_FOLDER_PATH.c_str(),
                         IN_CREATE | IN_MOVED_TO);

  return true;
}

namespace fs = std::filesystem;

void moveFile(const fs::path &source, const fs::path &dest) {
  try {
    if (!fs::exists(source))
      return;
    fs::create_directories(dest.parent_path()); // ensure destination exists
    fs::rename(source, dest);
    std::cout << "Moved: " << source << " -> " << dest << "\n";
  } catch (fs::filesystem_error &e) {
    std::cerr << "Error moving file: " << e.what() << "\n";
  }
}

int main() {

  init_file_watcher();
  const size_t event_size = sizeof(struct inotify_event);
  const size_t buf_len = 1024 * (event_size + 16);
  char buffer[buf_len];

  std::map<std::string, fs::path> filePaths = {
      {".jpg", fs::path("/home/" + USERNAME_STRING + "/Documents/Pictures/")},
      {".png", fs::path("/home/" + USERNAME_STRING + "/Documents/Pictures/")},
      {".pdf", fs::path("/home/" + USERNAME_STRING + "/Documents/PDFs/")},
  };

  while (true) {
    int length = read(fd, buffer, buf_len);
    if (length < 0) {
      perror("read");
      break;
    }

    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        fs::path filePath = DOWNLOADS_FOLDER_PATH + std::string(event->name);
        if (filePath.extension() != ".part" ||
            filePath.extension() != ".crdownload" || fs::exists(filePath)) {
          auto ext = filePath.extension().string();
          if (filePaths.count(ext)) {
            moveFile(filePath, filePaths[ext] / filePath.filename());
            printf("Moving file:%s ", filePath.c_str());
          }
        }
      }
      i += event_size + event->len;
    }
  }

  return 0;
}
