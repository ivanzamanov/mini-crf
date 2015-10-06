#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include<unistd.h>

#include<iostream>
#include<sstream>
#include<vector>
#include<string>

#include"util.hpp"

unsigned MIN_OPTS = 2;

enum Mode { SYNTH, QUERY, RESYNTH, TRAIN, BASELINE, INVALID };

Mode get_mode(std::string str) {
  if(str == "synth") return Mode::SYNTH;
  else if(str == "query") return Mode::QUERY;
  else if(str == "resynth") return Mode::RESYNTH;
  else if(str == "train") return Mode::TRAIN;
  else if(str == "baseline") return Mode::BASELINE;
  return Mode::INVALID;
}

struct Options {
  Options() { }

  Options(unsigned l, const char** argv) {
    for(unsigned i = 0; i < l; i++)
      args.push_back(argv[i]);

    if(isatty(STDIN_FILENO))
      return;
    
    std::string buffer;
    //std::cout << (char) std::cin.get() << std::endl;
    while(std::cin >> buffer) {
      int index = buffer.find_first_of('=');
      if(index >= 0) {
        args.push_back(std::string("--") + buffer.substr(0, index));
        args.push_back(buffer.substr(index + 1));
      } else {
        args.push_back(std::string("--") + buffer);
      }
    }
  }

  // Common
  std::string synth_db;
  std::string mode;
  std::string input;
  std::string test_db;

  // Synth
  std::string text_grid;

  // Query
  bool phon_id;
  bool sentence;
  bool concat_cost;
  bool text_input;

  std::vector<std::string> args;

  bool has_opt(std::string opt) const {
    opt = std::string("--") + opt;
    for(unsigned i = 0; i < args.size(); i++) {
      std::string test(args[i]);
      if(opt == test) {
        return true;
      }
    }
    return false;
  }

  std::string get_string(std::string opt, bool* found=0) const {
    if(found) *found = false;
    for(unsigned i = 0; i < args.size() - 1; i++) {
      std::string test(args[i]);
      if(opt == test) {
        std::string result(args[i + 1]);
        if(found) *found = true;
        INFO(opt << " = " << result);
        return result;
      }
    }
    return std::string("");
  }

  template<class T>
  T get_opt(std::string opt, T def) const {
    opt = std::string("--") + opt;
    bool found;
    std::string val = get_string(opt, &found);
    return found ? util::parse<T>(val) : def;
  }

  bool check_opt(std::string opt) const {
    if(!has_opt(opt)) {
      ERROR("Missing required option " << opt);
      return false;
    }
    return true;
  }
};

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    if(!(item == ""))
      elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> split_string(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

Options parse_options(unsigned argc, const char** argv) {
  Options opts(argc, argv);

  // common
  opts.synth_db = opts.get_opt<std::string>("synth-database", "");
  opts.test_db = opts.get_opt<std::string>("test-database", "");
  opts.mode = opts.get_opt<std::string>("mode", "");
  opts.input = opts.get_opt<std::string>("input", "");

  // synth
  opts.text_grid = opts.get_opt<std::string>("textgrid", "resynth.TextGrid");

  // query
  opts.phon_id = opts.has_opt("phonid");
  opts.sentence = opts.has_opt("sentence");
  opts.concat_cost = opts.has_opt("concat-cost");
  opts.text_input = opts.has_opt("text");

  return opts;
}

bool has_required(Options& opts) {
  return opts.check_opt("synth-database")
    && opts.check_opt("test-database");
}
#endif
