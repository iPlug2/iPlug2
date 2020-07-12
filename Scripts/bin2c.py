import argparse
import re
import sys

LINE_WRAP_CHARS = ', '
NON_NAME_PATTERN = re.compile(r'[^A-Za-z0-9_]')

def line_wrap(msg, max_len, indent):
  def rfind_any(chars, s, start, end):
    last = -1
    for c in chars:
      r = s.rfind(c, start, end)
      if r > last:
        last = r
        start = r
    return last
  
  def find_any(chars, s, start, end):
    first = end + 1
    for c in chars:
      r = s.find(c, start, end)
      if r < first:
        first = r
        end = r
    if first == end + 1:
      return -1
    else:
      return first

  indent_str = ' ' * indent
  out = []
  iStart = 0
  sEnd = len(msg) - max_len
  while iStart < sEnd:
    iEnd = iStart + max_len
    # Go to the end of a line and walk backwards until we find a char we can split on
    i = rfind_any(LINE_WRAP_CHARS, msg, iStart, iEnd)
    # If the line cannot be split (e.g. VERY long preset name), go forwards until we can split
    if i == -1:
      i = find_any(LINE_WRAP_CHARS, msg, iEnd, len(msg))
    # We STILL didn't find a split char. Just call it done.
    if i == -1:
      i = len(msg) - 1
    # We found a char to split at, but we want to split AFTER it  
    i += 1
    out.append(msg[iStart:i] + '\n' + indent_str)
    # And move over to the next part of the message
    iStart = i
    #msg = msg[i:]
  out.append(msg[iStart:])
  return ''.join(out)
    
def process(data, array_name):
  cname = re.sub(NON_NAME_PATTERN, '_', array_name)
  msg = []
  msg.append('const uint8_t %s[%d] = {' % (cname, len(data)))
  # Conver the bytes into a C array. Doing it this way helps slightly with performance
  i = 0
  end = len(data) - 4
  d = data
  while i < end:
    msg.append('%d,%d,%d,%d,' % (d[i+0], d[i+1], d[i+2], d[i+3]))
    i += 4
  # Finish it
  while i < len(d):
    msg.append('%d,' % d[i])
    i += 1
  msg.append('};\n')
  msg.append('const int %s_length = %d;\n\n' % (cname, len(data)))
  msg = ''.join(msg)
  return line_wrap(msg, 5000, 2)
  #return msg

def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('input_file', type=str,
    help='Input binary file')
  parser.add_argument('output_file', type=str,
    help='Output C/C++ file')
  parser.add_argument('array_name', type=str,
    help='Name of the data array')
  parser.add_argument('-c', '--compress', default='none',
    choices=['gzip', 'bz2', 'xz', 'none'],
    help='Compress the data before exporting it')
  args = parser.parse_args(argv)
  
  with open(args.input_file, 'rb') as fd:
    data_in = fd.read()
  
  # Handle compression
  comp = args.compress
  if comp == 'gzip':
    import gzip
    data = gzip.compress(data_in)
    
  elif comp == 'bz2':
    import bz2
    c = bz2.BZ2Compressor()
    data = c.compress(data_in)
    data += c.flush()
    
  elif comp == 'xz':
    import lzma
    c = lzma.LZMACompressor(format=lzma.FORMAT_XZ)
    data = c.compress(data_in)
    data += c.flush()
    
  elif comp == 'none':
    data = data_in
    
  else:
    raise Exception('Invalid compression method')
  
  # Export to C/C++
  with open(args.output_file, 'w') as fd:
    fd.write(process(data, args.array_name))

if __name__ == '__main__':
  main(sys.argv[1:])
