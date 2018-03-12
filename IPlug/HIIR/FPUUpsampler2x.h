/*
FPUUpsampler2x.h
Copyright (c) 2005 Laurent de Soras

Upsamples by a factor 2 the input signal, using FPU.

Template parameters:
- NC: number of coefficients, > 0

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*/

#pragma once

#include "Array.h"
#include "FPUStageProc.h"

namespace hiir
{

template <int NC>
class Upsampler2xFPU
{
public:

  enum { NBR_COEFS = NC };

  Upsampler2xFPU ();

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
  void set_coefs (const double coef_arr [NBR_COEFS]);

  /*
  Name: process_sample
  Description:
    Upsamples (x2) the input sample, generating two output samples.
  Input parameters:
    - input: The input sample.
  Output parameters:
    - out_0: First output sample.
    - out_1: Second output sample.
  */
  inline void process_sample (float &out_0, float &out_1, float input);

  /*
  Name: process_block
  Description:
    Upsamples (x2) the input sample block.
    Input and output blocks may overlap, see assert() for details.
  Input parameters:
    - in_ptr: Input array, containing nbr_spl samples.
    - nbr_spl: Number of input samples to process, > 0
  Output parameters:
    - out_0_ptr: Output sample array, capacity: nbr_spl * 2 samples.
  */
  void process_block (float out_ptr [], const float in_ptr [], long nbr_spl);

  /*
  Name: clear_buffers
  Description:
    Clears filter memory, as if it processed silence since an infinite amount
    of time.
  */
  void clear_buffers ();

private:
  Array <float, NBR_COEFS> _coef;
  Array <float, NBR_COEFS> _x;
  Array <float, NBR_COEFS> _y;

private:
  bool operator == (const Upsampler2xFPU &other);
  bool operator != (const Upsampler2xFPU &other);

};  // class Upsampler2xFPU

template <int NC>
Upsampler2xFPU <NC>::Upsampler2xFPU ()
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

template <int NC>
void Upsampler2xFPU <NC>::set_coefs (const double coef_arr [NBR_COEFS])
{
  assert (coef_arr != 0);

  for (int i = 0; i < NBR_COEFS; ++i)
  {
    _coef [i] = static_cast <float> (coef_arr [i]);
  }
}

template <int NC>
void Upsampler2xFPU <NC>::process_sample (float &out_0, float &out_1, float input)
{
  assert (&out_0 != 0);
  assert (&out_1 != 0);

  float even = input;
  float odd = input;
  StageProcFPU <NBR_COEFS>::process_sample_pos (
    NBR_COEFS,
    even,
    odd,
    &_coef [0],
    &_x [0],
    &_y [0]
  );
  out_0 = even;
  out_1 = odd;
}

template <int NC>
void Upsampler2xFPU <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl)
{
  assert (out_ptr != 0);
  assert (in_ptr != 0);
  assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
  assert (nbr_spl > 0);

  long pos = 0;
  do
  {
    process_sample (
      out_ptr [pos * 2],
      out_ptr [pos * 2 + 1],
      in_ptr [pos]
    );
    ++ pos;
  }
  while (pos < nbr_spl);
}

template <int NC>
void Upsampler2xFPU <NC>::clear_buffers ()
{
  for (int i = 0; i < NBR_COEFS; ++i)
  {
    _x [i] = 0;
    _y [i] = 0;
  }
}

} // namespace hiir
