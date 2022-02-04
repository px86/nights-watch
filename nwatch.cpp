#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " FILE COMMAND" << std::endl;
    std::exit(1);
  }

  // This is important.
  std::cout << "And now my watch begins..." << std::endl;
  atexit([](){ std::cout << "And now my watch ends..." << std::endl; });

  const char *filepath = argv[1];

  struct stat s;
  if (stat(filepath, &s) < 0) {
    std::perror("stat failed");
    std::exit(1);
  }
  timespec old = s.st_mtim;

  auto watcher = [&]() {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed");
      std::exit(1);
    }
    if (pid == 0) {
      if (execvp(argv[2], argv+2) < 0) {
	perror("exec failed");
	std::exit(1);
      }
    }
    while (true) {
      sleep(2);
      if (stat(filepath, &s) < 0) {
	std::perror("stat failed");
	std::exit(1);
      }
      if (s.st_mtim.tv_sec != old.tv_sec) {
	old = s.st_mtim;
	std::cout << "[Modified] " << s.st_mtim.tv_sec << '\n';

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
