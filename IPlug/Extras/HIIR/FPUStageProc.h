/*
        StageProcFPU.h
        Copyright (c) 2005 Laurent de Soras

Template parameters:
  - REMAINING: Number of remaining coefficients to process, >= 0

  --- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*/

#pragma once

namespace hiir
{

template <int REMAINING, typename T>
class StageProcFPU
{
public:
  static inline void process_sample_pos (const int nbr_coefs, T &spl_0, T &spl_1, const T coef [], T x [], T y []);
  static inline void process_sample_neg (const int nbr_coefs, T &spl_0, T &spl_1, const T coef [], T x [], T y []);

private:
  StageProcFPU();
  StageProcFPU(const StageProcFPU &other);
  StageProcFPU& operator = (const StageProcFPU &other);
  bool operator == (const StageProcFPU &other);
  bool operator != (const StageProcFPU &other);

};  // class StageProcFPU

template <>
inline void StageProcFPU <1, double>::process_sample_pos (const int nbr_coefs, double &spl_0, double &/*spl_1*/, const double coef [], double x [], double y [])
{
  const int last = nbr_coefs - 1;
  const double temp = (spl_0 - y [last]) * coef [last] + x [last];
  x [last] = spl_0;
  y [last] = temp;
  spl_0 = temp;
}

template <>
inline void StageProcFPU <0, double>::process_sample_pos (const int /*nbr_coefs*/, double &/*spl_0*/, double &/*spl_1*/, const double /*coef*/ [], double /*x*/ [], double /*y*/ [])
{
  // Nothing (stops recursion)
}
  
template <>
inline void StageProcFPU <1, float>::process_sample_pos (const int nbr_coefs, float &spl_0, float &/*spl_1*/, const float coef [], float x [], float y [])
{
  const int last = nbr_coefs - 1;
  const float temp = (spl_0 - y [last]) * coef [last] + x [last];
  x [last] = spl_0;
  y [last] = temp;
  spl_0 = temp;
}

template <>
inline void StageProcFPU <0, float>::process_sample_pos (const int /*nbr_coefs*/, float &/*spl_0*/, float &/*spl_1*/, const float /*coef*/ [], float /*x*/ [], float /*y*/ [])
{
  // Nothing (stops recursion)
}

template <int REMAINING, typename T>
void  StageProcFPU <REMAINING, T>::process_sample_pos (const int nbr_coefs, T &spl_0, T &spl_1, const T coef [], T x [], T y [])
{
  const int   cnt = nbr_coefs - REMAINING;

  const T   temp_0 =
    (spl_0 - y [cnt + 0]) * coef [cnt + 0] + x [cnt + 0];
  const T   temp_1 =
    (spl_1 - y [cnt + 1]) * coef [cnt + 1] + x [cnt + 1];

  x [cnt + 0] = spl_0;
  x [cnt + 1] = spl_1;

  y [cnt + 0] = temp_0;
  y [cnt + 1] = temp_1;

  spl_0 = temp_0;
  spl_1 = temp_1;

  StageProcFPU <REMAINING - 2, T>::process_sample_pos (
    nbr_coefs,
    spl_0,
    spl_1,
    &coef [0],
    &x [0],
    &y [0]
  );
}

template <>
inline void StageProcFPU <1, double>::process_sample_neg (const int nbr_coefs, double &spl_0, double &/*spl_1*/, const double coef [], double x [], double y [])
{
  const int last = nbr_coefs - 1;
  const double temp = (spl_0 + y [last]) * coef [last] - x [last];
  x [last] = spl_0;
  y [last] = temp;
  spl_0 = temp;
}

template <>
inline void StageProcFPU <0, double>::process_sample_neg (const int /*nbr_coefs*/, double &/*spl_0*/, double &/*spl_1*/, const double /*coef*/ [], double /*x*/ [], double /*y*/ [])
{
  // Nothing (stops recursion)
}
  
template <>
inline void StageProcFPU <1, float>::process_sample_neg (const int nbr_coefs, float &spl_0, float &/*spl_1*/, const float coef [], float x [], float y [])
{
  const int last = nbr_coefs - 1;
  const float temp = (spl_0 + y [last]) * coef [last] - x [last];
  x [last] = spl_0;
  y [last] = temp;
  spl_0 = temp;
}

template <>
inline void StageProcFPU <0, float>::process_sample_neg (const int /*nbr_coefs*/, float &/*spl_0*/, float &/*spl_1*/, const float /*coef*/ [], float /*x*/ [], float /*y*/ [])
{
  // Nothing (stops recursion)
}

template <int REMAINING, typename T>
void  StageProcFPU <REMAINING, T>::process_sample_neg (const int nbr_coefs, T &spl_0, T &spl_1, const T coef [], T x [], T y [])
{
  const int cnt = nbr_coefs - REMAINING;

  const T   temp_0 =
    (spl_0 + y [cnt + 0]) * coef [cnt + 0] - x [cnt + 0];
  const T   temp_1 =
    (spl_1 + y [cnt + 1]) * coef [cnt + 1] - x [cnt + 1];

  x [cnt + 0] = spl_0;
  x [cnt + 1] = spl_1;

  y [cnt + 0] = temp_0;
  y [cnt + 1] = temp_1;

  spl_0 = temp_0;
  spl_1 = temp_1;

  StageProcFPU <REMAINING - 2, T>::process_sample_neg (
    nbr_coefs,
    spl_0,
    spl_1,
    &coef [0],
    &x [0],
    &y [0]
  );
}


} // namespace hiir
