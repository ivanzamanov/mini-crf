#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include<sstream>
#include<vector>
#include<string>

int MIN_OPTS = 2;

enum Mode { SYNTH, QUERY, INVALID };

Mode get_mode(std::string str) {
  if(str == "synth") return Mode::SYNTH;
  else if(str == "query") return Mode::QUERY;
  return Mode::INVALID;
}

struct Options {
  Options(unsigned l, const char** args): length(l), args(args) { }

  // Common
  std::string db;
  std::string mode;
  std::string input;

  // Synth
  std::string text_grid;

  // Query
  bool phon_id;
  bool sentence;
  bool concat_cost;

  unsigned length;
  const char** args;

  bool has_opt(std::string opt) {
    for(unsigned i = 0; i < length; i++) {
      std::string test(args[i]);
      if(opt == test) {
        return true;
      }
    }
    return false;
  }

  std::string get_opt(std::string opt) {
    return get_opt(opt, 0);
  }

  std::string get_opt(std::string opt, bool* found) {
    if(found) *found = false;
    for(unsigned i = 0; i < length - 1; i++) {
      std::string test(args[i]);
      if(opt == test) {
        if(found) *found = true;
        return std::string(args[i + 1]);
      }
    }
    return std::string("");
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
  opts.db = opts.get_opt("--database");
  opts.mode = opts.get_opt("--mode");
  opts.input = opts.get_opt("--input");

  // synth
  opts.text_grid = opts.get_opt("--textgrid");

  // query
  opts.phon_id = opts.has_opt("--phonid");
  opts.sentence = opts.has_opt("--sentence");
  opts.concat_cost = opts.has_opt("--concat-cost");

  return opts;
}

#endif
