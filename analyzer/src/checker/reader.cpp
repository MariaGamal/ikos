#include <llvm/Support/CommandLine.h>
#include <fstream>
#include <ikos/analyzer/support/reader.hpp>
#include <iostream>

static llvm::cl::OptionCategory GradCategory("Grad Options");
static llvm::cl::opt< std::string > DataFilename(
    "lines",
    llvm::cl::desc("Specify the name of the data file"),
    llvm::cl::value_desc("filename"),
    llvm::cl::cat(GradCategory));

std::string read_file(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << file_path << std::endl;
    return "";
  }

  std::string content((std::istreambuf_iterator< char >(file)),
                      std::istreambuf_iterator< char >());
  file.close();

  return content;
}
