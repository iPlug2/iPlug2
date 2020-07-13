import argparse
import collections
import os
import os.path
import re
import sys

INDENT = 2
LINE_WRAP_CHARS = ', '
NON_NAME_PATTERN = re.compile(r'[^A-Za-z0-9_]')
SCALE_SIZE_PATTERN = re.compile(r'@[0-9]+')
Entry = collections.namedtuple('Entry', ['name', 'data', 'cname'])
STRUCT_DECL = '''struct {0} {{
{1}{0}(const char* name, const uint8_t* data, uint32_t size) : name(name), data(data), size(size) {{}}
{1}const char* name; const uint8_t* data; const uint32_t size;
}};'''

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
  
def make_cname(s):
  return re.sub(NON_NAME_PATTERN, '_', s)
  
def print_entry(en, array_type):
  return '{}(\"{}\", {}, {})'.format(array_type, en.name, en.cname, len(en.data))
    
def process(entries, src_file, header_file=None, array_name=None, line_length=5000):
  """
  Generate a source file and optional header file by converting a list of files into C arrays.
  :param entries: List of entries to include in the generated file
  :type entries: Entry[]
  :param src_file: string - Path to the cpp file that will be generated
  :param header_file: string|None - Path to the .h file to generate, None to not generate a header
  :param array_name: string|None - If this is supplied an array will be created with this name containing
    the name, pointer, and length of each data array.
  :param line_length: int - Maximum line length for line wrapping
  """
  
  array_type = 'resource_t'
  len_suffix = '_length'
  indent_str = ' ' * INDENT
  
  if array_name and not header_file:
    raise Exception("Building an array requires creating a header file.")
  
  with open(src_file, "w") as fd:
    # Add our include statement
    if header_file:
      fd.write('#include "%s"\n\n' % header_file)
    # Process each entry
    for en in entries:
      msg = []
      msg.append('const uint8_t %s[%d] = {' % (en.cname, len(en.data)))
      # Convert the bytes into a C array. Doing multiple at a time helps slightly with performance
      i = 0
      end = len(en.data) - 4
      d = en.data
      while i < end:
        msg.append('%d,%d,%d,%d,' % (d[i+0], d[i+1], d[i+2], d[i+3]))
        i += 4
      # Finish it
      while i < len(d):
        msg.append('%d,' % d[i])
        i += 1
      msg.append('};\n')
      msg.append('const int %s = %d;\n\n' % (en.cname + len_suffix, len(en.data)))
      msg = ''.join(msg)
      fd.write(line_wrap(msg, line_length, INDENT))
     
    # Create the array of data
    if array_name:
      msg = ''
      msg += 'const {} {}[] = {{\n'.format(array_type, array_name)
      for en in entries:
        msg += indent_str + print_entry(en, array_type) + ',\n'
      msg += '};\n'
      msg += 'const int %s = %d;\n' % (array_name + len_suffix, len(entries))
      fd.write(msg)
  # END with
  
  # Write the header file
  if header_file:
    with open(header_file, 'w') as fd:
      fd.write('#pragma once\n\n')
      # Guard for multiple headers defining resource_t
      guard_def = array_type.upper() + '_DEFINED'
      fd.write('#ifndef {0}\n#define {0}\n'.format(guard_def))
      fd.write(STRUCT_DECL.format(array_type, indent_str))
      fd.write('\n#endif\n\n')
      
      # First write each entry individually
      msg = ''
      for en in entries:
        msg += 'extern const uint8_t %s[%d];\n' % (en.cname, len(en.data))
        msg += 'extern const int %s;\n' % (en.cname + len_suffix)
      msg += '\n'
      fd.write(msg)
      
      # Now write the array extern, if it was asked for
      if array_name:
        msg  = 'extern const %s %s[%d]\n' % (array_type, array_name, len(entries))
        msg += 'extern const int %s;\n' % (array_name + len_suffix)
        fd.write(msg)
# END process


def compress(method, data_in):
  '''
  Compress data in one of a set of methods.
  '''
  data = None
  
  if method == 'gzip':
    import gzip
    data = gzip.compress(data_in)
    
  elif method == 'bz2':
    import bz2
    c = bz2.BZ2Compressor()
    data = c.compress(data_in)
    data += c.flush()
    
  elif method == 'xz':
    import lzma
    c = lzma.LZMACompressor(format=lzma.FORMAT_XZ)
    data = c.compress(data_in)
    data += c.flush()
    
  elif method == 'none':
    data = data_in
    
  else:
    raise Exception('Invalid compression method')
    
  return data
# END compress

def main(argv):
  parser = argparse.ArgumentParser()
  # parser.add_argument('-g', '--header', type=str, default=None,
  #   help='Output header file (default: none)')
  parser.add_argument('--cd', type=str, default='.',
    help='CD to the given directory before listing files.')
  parser.add_argument('-c', '--compress', default='none',
    choices=['gzip', 'bz2', 'xz', 'none'],
    help='Compress the data before exporting it')
  parser.add_argument('-a', '--array', type=str, default=None, metavar='name',
    help='If set this will generate an array of included resources and their names')
  parser.add_argument('-s', '--scaled', type=str, action='append', metavar='path', default=[],
    help='For each file specified here, bin2c will search for versions of the file with @2, @3, @4, etc. '
      + 'before the extension. These will all be added as resources.')
  parser.add_argument('output_file', type=str,
    help='Output C/C++ file')
  parser.add_argument('output_header', type=str,
    help='Output header file')
  parser.add_argument('inputs', type=str, nargs='*',
    metavar='file_name data_name',
    help='Pairs of input file names and C array names')
  args = parser.parse_args(argv)
  
  os.chdir(args.cd)
  
  inputs = args.inputs
  if len(inputs) % 2 != 0:
    raise Exception('Invalid number of input arguments, must be multiple of 2')
    
  entries = []
  
  def add_entry(path, name):
    pt = os.path.relpath(path).replace('\\', '/')
    with open(pt, 'rb') as fd:
      data_in = fd.read()
    data = compress(args.compress, data_in)
    entries.append(Entry(pt, data, make_cname(name)))
  
  # Process our normal args
  for i in range(0, len(inputs), 2):
    add_entry(inputs[i+0], inputs[i+1])
  
  # Process scaled arguments
  for pt in args.scaled:
    dir, fname = os.path.split(pt)
    froot, fext = os.path.splitext(fname)
    if dir == '':
      dir = '.'
    # Find all files that match the given file name format
    for fn in os.listdir(dir):
      if fn.startswith(froot) and fn.endswith(fext):
        m = SCALE_SIZE_PATTERN.match(fn[len(froot):-len(fext)])
        # If it's a scaled file or the current file, add it
        if m or fn == fname:
          add_entry(fn, fn)
  # end process scaled arguments
  
  process(entries, args.output_file, args.output_header, args.array)

if __name__ == '__main__':
  main(sys.argv[1:])
