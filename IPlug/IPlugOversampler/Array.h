/*
Array.h
Copyright (c) 2005 Laurent de Soras

Simple class wrapping static C arrays with bound check.

Template parameters:
- T: Contained class. Should have T::T() and T::~T()
- LENGTH: Number of contained elements. > 0.

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
template <class T, long LEN>
class Array
  {
public:
  typedef T Element;
  enum { LENGTH = LEN };
  Array () {};
  Array (const Array <T, LEN> &other);
  Array <T, LEN> &operator = (const Array <T, LEN> &other);
  const Element& operator [] (long pos) const;
  Element & operator [] (long pos);
  static long size ();
private:
  Element _data [LENGTH];
  bool operator == (const Array &other);
  bool operator != (const Array &other);

};  // class Array

template <class T, long LEN>
Array <T, LEN>::Array (const Array <T, LEN> &other)
{
  for (long pos = 0; pos < LENGTH; ++pos)
  {
    _data [pos] = other._data [pos];
  }
}

template <class T, long LEN>
Array <T, LEN> &  Array <T, LEN>::operator = (const Array <T, LEN> &other)
{
  for (long pos = 0; pos < LENGTH; ++pos)
  {
    _data [pos] = other._data [pos];
  }

  return (*this);
}

template <class T, long LEN>
const typename Array <T, LEN>::Element &  Array <T, LEN>::operator [] (long pos) const
{
  assert (pos >= 0);
  assert (pos < LENGTH);

  return (_data [pos]);
}

template <class T, long LEN>
typename Array <T, LEN>::Element &  Array <T, LEN>::operator [] (long pos)
{
  assert (pos >= 0);
  assert (pos < LENGTH);

  return (_data [pos]);
}

template <class T, long LEN>
long  Array <T, LEN>::size ()
{
  return (LENGTH);
}
} // namespace hiir

