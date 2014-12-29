#ifndef __CRF_H__
#define __CRF_H__

template<class _Item>
class Sequence {
  _Item atPosition(int);
  _Item operator[](int) {
    return atPosition(int);
  };
};

template<class _Label, class _Input>
class TransitionFunction {
  virtual double operator()(const _Label&, const _Label&, int, const Sequence<_Input>&);
};

template<class _Label, class _Input>
class StateFunction {
  virtual double operator()(const _Label&, int, const Sequence<_Input>&);
};

template<class Label, class ObservationSequence>
class LinearCRF {
public:
  LinearCRF();
  ~LinearCRF();
  LinearCRF(const& LinearCRF);
  const LinearCRF& operator=(const LinearCRF&);

  void serialize();

private:

};

#endif

