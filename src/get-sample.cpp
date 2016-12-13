#include<iostream>
#include<fstream>
#include<cmath>

#include"util.hpp"
#include"wav.hpp"

void doPrint(int, char** argv) {
  std::ifstream str1(argv[1]);
  Wave wav1(str1);

  auto sample = util::parse<int>(std::string(argv[2]));

  std::cout << wav1[sample] << std::endl;
}

bool Progress::enabled = false;
int main(int argc, char** argv) {
  doPrint(argc, argv);
}
