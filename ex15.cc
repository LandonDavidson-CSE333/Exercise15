// 2025 Copyright Landon Davidson
// landond@uw.edu

#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>

// Takes argc and argv and returns the hostname,
// port number, and opened file as output params
// Will print errors and return false on failure, true on success
bool processInput(int argc, char** argv, char** hostname, int *port, int *fd);

// Takes hostname and port and returns a
// connected socket fd through output param
// Will return false on failure with error messages to cerr, and true on success
bool openConnection(char *hostname, int port, int *socket_fd);

int main(int argc, char** argv) {
  // Attempt to process input and open file with processInput
  char* hostname;
  int port;
  int input;
  if (!processInput(argc, argv, &hostname,  &port, &input)) {
    // Error messages were printed by processInput
    return EXIT_FAILURE;
  }

  // Attempt to open connection to hostname at port
  int socket;
  if (!openConnection(hostname, port, &socket)) {
    // Error messages were printed by openConnection
    close(input);
    return EXIT_FAILURE;
  }

  // Read from file and write to connection
  char buf[BUFSIZ];
  size_t num_read;
  // Repeatedly read BUFSIZ bytes into buf and write back into the socket
  while ((num_read = read(input, buf, BUFSIZ)) != 0) {
    // Check for read errors
    if (num_read == -1) {
      // If it was an interrupt try again, otherwise quit
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "Error reading from " << argv[3] << ": ";
      perror("");
      close(socket);
      close(input);
      return EXIT_FAILURE;
    }

    // Attempt to write the bytes we read to the server
    size_t bytes_left = num_read;
    while (bytes_left > 0) {
      size_t num_wrote = write(socket, buf, num_read);
      // If write failed give a message and close gracefully
      if (num_wrote == 0) {
        std::cerr << "Connection closed prematurely\n";
        close(socket);
        close(input);
        return EXIT_FAILURE;
      }
      if (num_wrote == -1) {
        // If it was an interrupt then try again
        if (errno == EINTR) {
          continue;
        }
        std::cerr << "Failed to write to connection: "
                  << strerror(errno) << std::endl;
        close(socket);
        close(input);
        return EXIT_FAILURE;
      }
      bytes_left -= num_wrote;
    }
  }
  // Close connection and clean up
  close(socket);
  close(input);
  return EXIT_SUCCESS;
}

bool processInput(int argc, char **argv, char** hostname, int *port, int *fd) {
  // Ensure we have the 3 arguments
  if (argc != 4) {
    std::cerr << "ex15 requires 3 arguments: hostname port local_file\n";
    return false;
  }

  // Copy hostname to output
  *hostname = argv[1];

  // Convert port to a short
  if (sscanf(argv[2], "%hu", port) != 1) {
    // second arg wasn't a short
    std::cerr << "Please enter a port number between 1024 and 65535\n";
    return false;
  }

  // Attempt to open file for reading
  *fd = open(argv[3], O_RDONLY);
  if (*fd == -1) {
    std::cerr << "Failed to open file " << argv[3] << " for reading: ";
    perror("");
    return false;
  }
  return true;
}

bool openConnection(char *hostname, int port, int *socket_fd) {
  // Get sockaddr for provided hostname and port
  addrinfo hints = {}, *results;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int addr_ret;
  if ((addr_ret = getaddrinfo(hostname, nullptr, &hints, &results)) != 0) {
    // If getaddrinfo fails print an error withgai_tstrerror
    std::cerr << "Failed to get address for hostname " << hostname
              << " with error: " << gai_strerror(addr_ret) << std::endl;
    return false;
  }

  // Add port number to results depending on familt
  if (results->ai_family == AF_INET6) {
    sockaddr_in6 *v6 = reinterpret_cast<sockaddr_in6*>(results->ai_addr);
    v6->sin6_port = htons(port);
  } else if (results-> ai_family == AF_INET) {
    sockaddr_in *v4 = reinterpret_cast<sockaddr_in*>(results->ai_addr);
    v4->sin_port = htons(port);
  } else {
    // Fail if we don't recognize the ip family
    std::cerr << "IP family " << results->ai_family << " not recognized\n";
    return false;
  }
  // Copy results to a sockaddr_storage and free results
  sockaddr_storage server_addr;
  memcpy(&server_addr, results->ai_addr, results->ai_addrlen);
  size_t server_addr_len = results->ai_addrlen;
  freeaddrinfo(results);

  // Attempt to create socket
  *socket_fd = socket(server_addr.ss_family, SOCK_STREAM, 0);
  if (*socket_fd == -1) {
    // Failed to create socket
    std::cerr << "Failed to create socket for connection\n";
    return false;
  }

  // Attempt to connect server to socket
  int connection_res = connect(*socket_fd,
          reinterpret_cast<sockaddr*>(&server_addr), server_addr_len);
  if (connection_res == -1) {
    // Failed to connect to hostname
    std::cerr << "Connection failed: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

