#include<sstream>
#include<string>
#include<cstring>

#include"parser.hpp"

static void check_buffer(const std::string& expected, const std::string& actual) {
  if(actual.compare(expected)) {
    std::cerr << "Expected: " << expected << " got: " << actual << '\n';
  }
}

static std::string buffer;
static void section(std::istream& stream, const std::string& check) {
  stream >> buffer;
  check_buffer(check, buffer);
}

template<class T>
static T value(std::istream& stream, const std::string& check) {
  T result;
  stream >> buffer;
  int index = buffer.find_first_of('=');
  check_buffer(check, buffer.substr(0, index));
  std::stringstream string_stream(buffer.substr(index + 1));
  string_stream >> result;
  return result;
}

std::string string_value(std::istream& stream, const std::string& check) {
  std::string result;
  char char_buffer[1024 * 1024];
  char c = stream.get();
  if(c == '\r')
    c = stream.get();
  stream.getline(char_buffer, sizeof(char_buffer));
  std::string buffer(char_buffer);
  int index = buffer.find_first_of('=');
  check_buffer(check, buffer.substr(0, index));
  result = buffer.substr(index + 1);
  return result;
}

PhonemeInstance* parse_file(std::istream& stream, int& size, StringLabelProvider& label_provider) {
  section(stream, "[Config]"); // [Config]
  value<double>(stream, "timeStep"); // timeStep
  value<int>(stream, "mfcc");
  size = value<int>(stream, "intervals");
  PhonemeInstance* result = new PhonemeInstance[size];

  for (int i = 0; i < size; i++) {
    section(stream, "[Entry]");
    result[i].label = label_provider.fromString(value<std::string>(stream, "label"));
    result[i].start = value<double>(stream, "start");
    result[i].end = value<double>(stream, "end");
    int frames = value<int>(stream, "frames");
    double duration = value<double>(stream, "duration");
    result[i].duration = duration;
    //result[i].frames.length = frames;
    if(frames < 0)
      std::cerr << "Error at " << stream.tellg() << std::endl;
    //result[i].frames.data = new Frame[frames];

    for(int frame = 0; frame < frames; frame++) {
      int c;

      result[i].frames[frame].pitch = value<double>(stream, "pitch");
      std::string mfcc_string = string_value(stream, "mfcc");
      std::stringstream mfcc_value_str(mfcc_string);
      for(c = 0; c < MFCC_N; c++) {
        mfcc_value_str >> result[i].frames[frame].mfcc[c];
      }
    }
  }

  return result;
}

BinaryWriter& operator<<(BinaryWriter& str, const PhonemeInstance& ph) {
  str << ph.id;
  str << ph.frames.size();
  for(unsigned i = 0; i < ph.frames.size(); i++)
    str << ph.frames[i];

  str << ph.start;
  str << ph.end;
  str << ph.duration;
  str << ph.label;
  return str;
}

BinaryReader& operator>>(BinaryReader& str, PhonemeInstance& ph) {
  unsigned length;
  str >> ph.id;
  str >> length;
  //ph.frames.data = new Frame[length];
  //ph.frames.length = length;
  for(unsigned i = 0; i < length; i++)
    str >> ph.frames[i];

  str >> ph.start;
  str >> ph.end;
  str >> ph.duration;
  str >> ph.label;
  return str;
}

bool compare(FrameArray& a1, FrameArray& a2) {
  bool same = a1.size() == a2.size();
  for(unsigned i = 0; i < a1.size(); i++)
    same = same && compare(a1[i], a2[i]);
  return same;
}

bool compare(Frame& p1, Frame& p2) {
  bool same = p1.pitch == p2.pitch;
  for(unsigned i = 0; i < p1.mfcc.size(); i++)
    same = same && p1.mfcc[i] == p2.mfcc[i];
  return same;
}

bool compare(PhonemeInstance& p1, PhonemeInstance& p2) {
  return compare(p1.frames, p2.frames) && p1.start == p2.start && p1.end == p2.end && p1.label == p2.label;
}

unsigned occurrence_count(char c, const std::string& str) {
  unsigned result = 0;
  for(unsigned i = 0; i < str.size(); i++)
    result += (str[i] == c);
  return result;
}

bool operator==(const Frame& f1, const Frame& f2) {
  bool same = f1.pitch == f2.pitch;
  
  for(unsigned i = 0; i < f1.mfcc.size() && same; i++)
    same &= f1.mfcc[i] == f2.mfcc[i];
  return same;
}

bool operator==(const PhonemeInstance& p1, const PhonemeInstance& p2) {
  bool same = p1.start == p2.start && p1.end == p2.end && p1.label == p2.label && p1.id == p2.id;
  same &= p1.frames.size() == p2.frames.size();
  for(unsigned i = 0; i < p1.frames.size() && same; i++)
    same &= p1.at(i) == p2.at(i);
  return same;
}

static std::string CSV_HEADER("id,duration,pitch");
bool check_csv(std::istream& is) {
  std::string buffer;
  std::getline(is, buffer);

  if(buffer[buffer.size() - 1] == '\r') // Windows sh**...
    buffer.pop_back();

  bool result = buffer == CSV_HEADER ? 3 : 0;
  if(!result)
    std::cerr << "Bad header: " << buffer << '\n';
  return result;
}

std::vector<PhonemeInstance> parse_synth_input_csv(std::istream& is) {
  std::vector<PhonemeInstance> result;

  unsigned cols = check_csv(is);
  if(!cols) {
    std::cerr << "Bad CSV input\n";
    return result;
  }

  std::string buffer;
  char id;
  while(is >> id) {
    char c;
    double duration, pitch;

    is >> c >> duration >> c >> pitch;
    PhonemeInstance p;
    p.label = id;
    p.start = 0;
    p.end = duration;
    p.duration = duration;
    //singleton_array(p.frames, Frame(pitch));
    p.frames[0] = Frame(pitch);
    result.push_back(p);
  }

  return result;
}

void print_synth_input_csv_phoneme(std::ostream& os, const PhonemeInstance& p) {
  double pitch = to_pitch_contour<false>(p)[0];
  os << p.label << ',' << p.duration << ',' << pitch;
}

void print_synth_input_csv(std::ostream& os, std::vector<PhonemeInstance>& phons) {
  os << CSV_HEADER << '\n';
  auto it = phons.begin();
  print_synth_input_csv_phoneme(os, *it);

  for(; it != phons.end(); it++) {
    os << '\n';
    print_synth_input_csv_phoneme(os, *it);
  }
}
