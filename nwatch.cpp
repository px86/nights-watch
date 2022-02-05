#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  auto print_usage_and_exit = [argv]() {
    std::cout << "Usage: " << argv[0] << " FILE [FILE]* -- COMMAND" << std::endl;
    std::exit(1);
  };

  if (argc < 4) print_usage_and_exit();

  // This is important.
  std::cout << "\x1b[32m\n" "And now my watch begins..." "\x1b[m\n\n";
  atexit([]() { std::cout << "\x1b[33m\n" "And now my watch ends..." "\x1b[m\n\n"; });

  auto filepaths = std::vector<const char*>();

  int cmd_offset = -1;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--")) {
      cmd_offset = ++i;
      break;
    }
    filepaths.push_back(argv[i]);
  }

  if (cmd_offset == -1 || cmd_offset == argc)
    print_usage_and_exit();

  auto file_stats = std::vector<struct stat>(filepaths.size());
  auto restat_files = [&filepaths, &file_stats]()
  {
    for (size_t i=0; i<filepaths.size(); ++i) {
      if (stat(filepaths[i], &file_stats[i]) < 0) {
	std::perror("\x1b[31m" "stat failed" "\x1b[m");
	std::exit(1);
      }
    }
  };
  restat_files();

  auto last_mtimes = std::vector<timespec>(file_stats.size());
  auto update_last_mtimes = [&file_stats, &last_mtimes]()
  {
    for (size_t i=0; i<file_stats.size(); ++i) {
      last_mtimes[i] = file_stats[i].st_mtim;
    }
  };
  update_last_mtimes();

  auto compare_mtimes = [&last_mtimes, &file_stats, &update_last_mtimes]() -> bool
  {
    for (size_t i=0; i<file_stats.size(); ++i) {
      if (last_mtimes[i].tv_sec != file_stats[i].st_mtim.tv_sec) {
	update_last_mtimes();
	return true;
      }
    }
    return false;
  };

  auto watcher = [&]() {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed");
      std::exit(1);
    }
    if (pid == 0) {
      if (execvp(argv[cmd_offset], argv+cmd_offset) < 0) {
	perror("exec failed");
	std::exit(1);
      }
    }
    while (true) {
      sleep(2);
      restat_files();
      if (compare_mtimes()) {
        // Kill the child process
        if (kill(pid, SIGKILL) < 0) {
	  perror("kill failed");
	  std::exit(1);
	}
	int wstatus;
	wait(&wstatus); // Handling zombies.
	break;
      }
    }
  };
  while (true) { watcher(); }

  return 0;
}
