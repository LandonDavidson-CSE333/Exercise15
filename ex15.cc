// 2025 Copyright Landon Davidson
// landond@uw.edu

#include <iostream>

int main (int argc, char** argv) {
  // Ensure we have the 3 arguments
  if (argc != 4) {
    std::cerr << "ex15 requires 3 arguments: hostname port local_file";
  }
  // Attempt to open file for reading
  FILE *input = fopen(argv[3], "r");
  if (input == nullptr) {
    std::cerr << "Failed to open file " << argv[3] << " for reading: ";
    perror("");
  }
}
