#include <fstream>
#include <iostream>
#include <regex>
#include <string>

class Parser {
private:
  std::string file;

public:
  Parser(std::string file_name) {
    std::ifstream file_stream(file_name);
    std::string content((std::istreambuf_iterator< char >(file_stream)),
                        (std::istreambuf_iterator< char >()));
    file = content;
  }

  std::vector< std::string > func_name() {
    std::regex pattern("define\\s+\\w+\\s+@\\w+");
    std::smatch matches;
    std::vector< std::string > func_names;
    while (std::regex_search(file, matches, pattern)) {
      func_names.push_back(matches[0].str().substr(7));
      file = matches.suffix().str();
    }
    return func_names;
  }
};

int main() {
  Parser parser("ar-sample");
  std::vector< std::string > func_names = parser.func_name();
  for (int i = 0; i < func_names.size(); i++) {
    std::cout << func_names[i] << std::endl;
  }

  return 0;
}
