/*

FPUDownsampler2x.h
Copyright(c) 2005 Laurent de Soras

Downsamples the input signal by a factor 2, using FPU.

Template parameters:
  - NC: number of coefficients, > 0

  --- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*/

#include <array>
#include "FPUStageProc.h"

namespace hiir
{
template <int NC, typename T>
class Downsampler2xFPU
{
public:
  enum { NBR_COEFS = NC };
  Downsampler2xFPU();

  /*
  Name: set_coefs
  Description:
  Sets filter coefficients. Generate them with the PolyphaseIir2Designer
  class.
  Call this function before doing any processing.
  Input parameters:
  - coef_arr: Array of coefficients. There should be as many coefficients as
  mentioned in the class template parameter.
  */
  void set_coefs(const double coef_arr[]);

  /*
  Name: process_sample
  Description:
  Downsamples (x2) one pair of samples, to generate one output sample.
  Input parameters:
  - in_ptr: pointer on the two samples to decimate
  Returns: Samplerate-reduced sample.
  */
  inline T process_sample(const T in_ptr [2]);

  /*
  Name: process_block
  Description:
  Downsamples (x2) a block of samples.
  Input and output blocks may overlap, see assert() for details.
  Input parameters:
  - in_ptr: Input array, containing nbr_spl * 2 samples.
  - nbr_spl: Number of samples to output, > 0
  Output parameters:
  - out_ptr: Array for the output samples, capacity: nbr_spl samples.
  */
  void process_block(T out_ptr[], const T in_ptr[], long nbr_spl);
  
  /*
   Name: process_sample_split
   Description:
   Split (spectrum-wise) in half a pair of samples. The lower part of the
   spectrum is a classic downsampling, equivalent to the output of
   process_sample().
   The higher part is the complementary signal: original filter response
   is flipped from left to right, becoming a high-pass filter with the same
   cutoff frequency. This signal is then critically sampled (decimation by 2),
   flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
   Input parameters:
   - in_ptr: pointer on the pair of input samples
   Output parameters:
   - low: output sample, lower part of the spectrum (downsampling)
   - high: output sample, higher part of the spectrum.
   */
  inline void process_sample_split(T &low, T &high, const T in_ptr[2]);

  /*
  Name: process_block_split
  Description:
   Split (spectrum-wise) in half a block of samples. The lower part of the
   spectrum is a classic downsampling, equivalent to the output of
   process_block().
   The higher part is the complementary signal: original filter response
   is flipped from left to right, becoming a high-pass filter with the same
   cutoff frequency. This signal is then critically sampled (decimation by 2),
   flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
  Input and output blocks may overlap, see assert() for details.
  Input parameters:
  - in_ptr: Input array, containing nbr_spl * 2 samples.
  - nbr_spl: Number of samples for each output, > 0
  Output parameters:
  - out_l_ptr: Array for the output samples, lower part of the spectrum
      (downsampling). Capacity: nbr_spl samples.
  - out_h_ptr: Array for the output samples, higher part of the spectrum.
      Capacity: nbr_spl samples.
  */
  void process_block_split(T out_l_ptr[], T out_h_ptr[], const T in_ptr[], long nbr_spl);

  /*
  Name: clear_buffers
  Description:
  Clears filter memory, as if it processed silence since an infinite amount
  of time.
  */
  void clear_buffers();

private:
  std::array<T, NBR_COEFS> _coef;
  std::array<T, NBR_COEFS> _x;
  std::array<T, NBR_COEFS> _y;

private:
  bool operator == (const Downsampler2xFPU &other);
  bool operator != (const Downsampler2xFPU &other);

};  // class Downsampler2xFPU


template <int NC, typename T>
Downsampler2xFPU <NC, T>::Downsampler2xFPU ()
: _coef ()
, _x ()
, _y ()
{
  for (int i = 0; i < NBR_COEFS; ++i)
  {
    _coef [i] = 0;
  }
  clear_buffers ();
}

template <int NC, typename T>
void  Downsampler2xFPU <NC, T>::set_coefs (const double coef_arr[])
{
  assert (coef_arr != 0);

  for (int i = 0; i < NBR_COEFS; ++i)
  {
    _coef [i] = static_cast <T> (coef_arr [i]);
  }
}

template <int NC, typename T>
T Downsampler2xFPU <NC, T>::process_sample (const T in_ptr [2])
{
  assert (in_ptr != 0);

  T spl_0 (in_ptr [1]);
  T spl_1 (in_ptr [0]);

  #if defined (_MSC_VER)
    #pragma inline_depth (255)
  #endif  // _MSC_VER

  StageProcFPU <NBR_COEFS, T>::process_sample_pos (
    NBR_COEFS,
    spl_0,
    spl_1,
    &_coef [0],
    &_x [0],
    &_y [0]
  );

  return (0.5f * (spl_0 + spl_1));
}


template <int NC, typename T>
void Downsampler2xFPU <NC, T>::process_block (T out_ptr[], const T in_ptr[], long nbr_spl)
{
  assert (in_ptr != 0);
  assert (out_ptr != 0);
  assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
  assert (nbr_spl > 0);

  long pos = 0;
  do
  {
    out_ptr [pos] = process_sample (&in_ptr [pos * 2]);
    ++pos;
  }
  while (pos < nbr_spl);
}

template <int NC, typename T>
void Downsampler2xFPU <NC, T>::process_sample_split (T &low, T &high, const T in_ptr [2])
{
  assert (&low != 0);
  assert (&high != 0);
  assert (in_ptr != 0);

  T       spl_0 = in_ptr [1];
  T       spl_1 = in_ptr [0];

  #if defined (_MSC_VER)
    #pragma inline_depth (255)
  #endif  // _MSC_VER

  StageProcFPU <NBR_COEFS, T>::process_sample_pos (
    NBR_COEFS,
    spl_0,
    spl_1,
    &_coef [0],
    &_x [0],
    &_y [0]
  );

  low = (spl_0 + spl_1) * 0.5f;
  high = spl_0 - low; // (spl_0 - spl_1) * 0.5f;
}

template <int NC, typename T>
void Downsampler2xFPU <NC, T>::process_block_split (T out_l_ptr[], T out_h_ptr[], const T in_ptr[], long nbr_spl)
{
  assert (in_ptr != 0);
  assert (out_l_ptr != 0);
  assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 2);
  assert (out_h_ptr != 0);
  assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 2);
  assert (out_h_ptr != out_l_ptr);
  assert (nbr_spl > 0);

  long pos = 0;
  do
  {
    process_sample_split
    (
      out_l_ptr [pos],
      out_h_ptr [pos],
      &in_ptr [pos * 2]
    );
    ++pos;
  }
  while (pos < nbr_spl);
}

template <int NC, typename T>
void Downsampler2xFPU <NC, T>::clear_buffers ()
{
  for (int i = 0; i < NBR_COEFS; ++i)
  {
    _x [i] = 0;
    _y [i] = 0;
  }
}

} // namespace hiir

