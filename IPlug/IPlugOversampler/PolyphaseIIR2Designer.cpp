/*
        PolyphaseIIR2Designer.cpp
        Copyright (c) 2005 Laurent de Soras

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*/

#if defined (_MSC_VER)
  #pragma warning (1 : 4130) // "'operator' : logical operation on address of string constant"
  #pragma warning (1 : 4223) // "nonstandard extension used : non-lvalue array converted to pointer"
  #pragma warning (1 : 4705) // "statement has no effect"
  #pragma warning (1 : 4706) // "assignment within conditional expression"
  #pragma warning (4 : 4786) // "identifier was truncated to '255' characters in the debug information"
  #pragma warning (4 : 4800) // "forcing value to bool 'true' or 'false' (performance warning)"
  #pragma warning (4 : 4355) // "'this' : used in base member initializer list"
#endif

#include  "PolyphaseIIR2Designer.h"

#include  <cassert>
#include  <cmath>

namespace hiir
{
  static const double PI = 3.141592653589793238;
  
  int round_int (double x)
  {
    return (static_cast <int> (std::floor (x + 0.5)));
  }
  
  int ceil_int (double x)
  {
    return (static_cast <int> (std::ceil (x)));
  }
  
  template <class T>
  T ipowp (T x, long n)
  {
    assert (n >= 0);
    
    T z (1);
    while (n != 0)
    {
      if ((n & 1) != 0)
      {
        z *= x;
      }
      n >>= 1;
      x *= x;
    }
    
    return (z);
  }

  
int PolyphaseIIR2Designer::compute_nbr_coefs_from_proto (double attenuation, double transition)
{
  assert (attenuation > 0);
  assert (transition > 0);
  assert (transition < 0.5);

  double k;
  double q;
  compute_transition_param (k, q, transition);
  const int   order = compute_order (attenuation, q);
  const int   nbr_coefs = (order - 1) / 2;

  return (nbr_coefs);
}

double  PolyphaseIIR2Designer::compute_atten_from_order_tbw (int nbr_coefs, double transition)
{
  assert (nbr_coefs > 0);
  assert (transition > 0);
  assert (transition < 0.5);

  double k;
  double q;
  compute_transition_param (k, q, transition);
  const int   order = nbr_coefs * 2 + 1;
  const double  attenuation = compute_atten (q, order);

  return (attenuation);
}

int PolyphaseIIR2Designer::compute_coefs (double coef_arr[], double attenuation, double transition)
{
  assert (&coef_arr != 0);
  assert (attenuation > 0);
  assert (transition > 0);
  assert (transition < 0.5);

  double k;
  double q;
  compute_transition_param (k, q, transition);

  // Computes number of required coefficients
  const int   order = compute_order (attenuation, q);
  const int   nbr_coefs = (order - 1) / 2;

  // Coefficient calculation
  for (int index = 0; index < nbr_coefs; ++index)
  {
    coef_arr [index] = compute_coef (index, k, q, order);
  }

  return (nbr_coefs);
}

void PolyphaseIIR2Designer::compute_coefs_spec_order_tbw (double coef_arr[], int nbr_coefs, double transition)
{
  assert (&coef_arr != 0);
  assert (nbr_coefs > 0);
  assert (transition > 0);
  assert (transition < 0.5);

  double k;
  double q;
  compute_transition_param (k, q, transition);
  const int   order = nbr_coefs * 2 + 1;

  // Coefficient calculation
  for (int index = 0; index < nbr_coefs; ++index)
  {
    coef_arr [index] = compute_coef (index, k, q, order);
  }
}
  
void PolyphaseIIR2Designer::compute_transition_param (double &k, double &q, double transition)
{
  assert (&k != 0);
  assert (&q != 0);
  assert (transition > 0);
  assert (transition < 0.5);

  using namespace std;

  k = tan ((1 - transition * 2) * hiir::PI / 4);
  k *= k;
  assert (k < 1);
  assert (k > 0);
  double      kksqrt = pow (1 - k * k, 0.25);
  const double  e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
  const double  e2 = e * e;
  const double  e4 = e2 * e2;
  q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
  assert (q > 0);
}

int PolyphaseIIR2Designer::compute_order (double attenuation, double q)
{
  assert (attenuation > 0);
  assert (q > 0);

  using namespace std;

  const double  attn_p2 = pow (10.0, -attenuation / 10);
  const double  a = attn_p2 / (1 - attn_p2);
  int       order = hiir::ceil_int (log (a * a / 16) / log (q));
  if ((order & 1) == 0)
  {
    ++ order;
  }
  if (order == 1)
  {
    order = 3;
  }

  return (order);
}

double  PolyphaseIIR2Designer::compute_atten (double q, int order)
{
  assert (q > 0);
  assert (order > 0);
  assert ((order & 1) == 1);
  
  using namespace std;
  
  const double  a = 4 * exp (order * 0.5 * log (q));
  assert (a != -1.0);
  const double attn_p2 = a / (1 + a);
  const double attenuation = -10 * log10 (attn_p2);
  assert (attenuation > 0);
  
  return (attenuation);
}
  
double  PolyphaseIIR2Designer::compute_coef (int index, double k, double q, int order)
{
  assert (index >= 0);
  assert (index * 2 < order);

  using namespace std;

  const int   c = index + 1;
  const double  num = compute_acc_num (q, order, c) * pow (q, 0.25);
  const double  den = compute_acc_den (q, order, c) + 0.5;
  const double  ww = num / den;
  const double  wwsq = ww * ww;

  const double  x = sqrt ((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
  const double  coef = (1 - x) / (1 + x);

  return (coef);
}

double  PolyphaseIIR2Designer::compute_acc_num (double q, int order, int c)
{
  assert (c >= 1);
  assert (c < order * 2);

  using namespace std;

  int i = 0;
  int j = 1;
  double acc = 0;
  double q_ii1;
  do
  {
    q_ii1 = hiir::ipowp (q, i * (i + 1));
    q_ii1 *= sin ((i * 2 + 1) * c * hiir::PI / order) * j;
    acc += q_ii1;

    j = -j;
    ++i;
  }
  while (fabs (q_ii1) > 1e-100);

  return (acc);
}

double  PolyphaseIIR2Designer::compute_acc_den (double q, int order, int c)
{
  assert (c >= 1);
  assert (c < order * 2);

  using namespace std;

  int i = 1;
  int j = -1;
  double acc = 0;
  double q_i2;
  do
  {
    q_i2 = hiir::ipowp (q, i * i);
    q_i2 *= cos (i * 2 * c * hiir::PI / order) * j;
    acc += q_i2;

    j = -j;
    ++i;
  }
  while (fabs (q_i2) > 1e-100);

  return (acc);
}

} // namespace hiir

