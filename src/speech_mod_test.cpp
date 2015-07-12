#include<cmath>

#include"speech_mod.hpp"
#include"util.hpp"

using std::vector;
using namespace util;

typedef SpeechWaveData STSignal;
extern vector<SpeechWaveData> extractSTSignals(const SpeechWaveData& swd);
extern void gen_window(float* data, int size);
extern vector<int> create_plan(vector<STSignal>& stSignals, const PhonemeInstance& tgt);

template<class T>
void assertEquals(std::string msg, T expected, T actual) {
  if(!(expected == actual)) {
    std::cerr << msg << ": "
              << "Expected: " << expected
              << " Actual: " << actual << std::endl;
    throw "Assert failed";
  }
}

void testSTExtraction() {
  const int PERIOD = 6;
  const int N = PERIOD * 10 + 1; short data[N];
  SpeechWaveData swd(WaveData(data, 0, N));
  transform(swd.data, N, [&](int i, short&) {
      if(i % PERIOD == 0) {
        swd.marks.push_back(i);
        std::cerr << "Mark at " << i << std::endl;
      }
      return 2;
    });

  std::cerr << swd.marks.size() << " marks" << std::endl;
  vector<STSignal> signals = extractSTSignals(swd);
  int window_size = 2 * PERIOD;
  float window[window_size]; gen_window(window, window_size);

  assertEquals("Signals equal marks - 2", swd.marks.size() - 2, signals.size());

  each2(signals, [&](int, STSignal& signal) {
      assertEquals("STS length", window_size, signal.length);

      for(int i = 0; i < signal.length; i++)
        assertEquals("STS value", (short) (window[i] * 2), signal[i]);

      assertEquals("STS mark", signal.length / 2, signal.marks[0]);
    });
}

void testPlanCreation() {
  PhonemeInstance target;
  target.start = 0; target.end = 1; target.duration = 1;
  target.pitch_contour[0] = 90;
  target.pitch_contour[1] = 110;
  vector<STSignal> stSignals;stSignals.resize(2);
  each(stSignals, [](STSignal& s) { s.length = DEFAULT_SAMPLE_RATE * 0.3f; });

  float targetPitch = (target.pitch_contour[0] + target.pitch_contour[1]) / 2;

  vector<int> plan = create_plan(stSignals, target);
  double targetDur = target.duration;
  for(unsigned i = 0; i < plan.size(); i++) {
    while(plan[i] > 0) {
      targetDur -= 1 / targetPitch;
      plan[i]--;
    }
  }
  std::cerr << "Target duration: " << target.duration << std::endl;
  std::cerr << "Complete duration: " << targetDur << std::endl;
}

int main() {
  try {
    std::cerr << "Test ST Extraction" << std::endl;
    testSTExtraction();
    std::cerr << "Test Plan Creation" << std::endl;
    testPlanCreation();
  } catch(char const*) {

  }
}
