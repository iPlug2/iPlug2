import argparse
import collections
from os.path import join, normpath
import re
import sys



class BinToC:
  LINE_WRAP_CHARS = ' ,'
  NON_NAME_PATTERN = re.compile(r'[^A-Za-z0-9_]')
  Entry = collections.namedtuple('Entry', ['name', 'data', 'cname'])
  STRUCT_DECL = '''struct {0} {{
{1}{0}(const char* name, const uint8_t* data, uint32_t size) : name(name), data(data), size(size) {{}}
{1}const char* name; const uint8_t* data; const uint32_t size;
}};'''

  def __init__(self, max_line=120, indent=2, prefix=''):
    self.max_line = max_line
    self.indent = indent
    self.prefix = ''
    self.index_type = 'resource_data_t'
    self.index_name = 'RESOURCE_LIST'
    self.index_list = True
    self.data_suffix = '_data'
    self.entries = []
  
  def add(self, name, data, prefix=None):
    cname = re.sub(BinToC.NON_NAME_PATTERN, '_', (prefix or self.prefix) + name)
    self.entries.append(self.Entry(name, data, cname))
  
  def generate(self, out_dir, out_name):
    h_name = out_name + '.h'
    cpp_name = out_name + '.cpp'
    
    with open(join(out_dir, cpp_name), 'w') as fd:
      # Write the header
      fd.write('#include "{}"\n\n'.format(h_name))

      # Write each entry
      for en in self.entries:
        msg = 'const uint8_t %s[%d] = {' % (en.cname + self.data_suffix, len(en.data))
        # Conver the bytes into a C array
        for b in en.data:
          msg += str(b) + ','
        msg += '};\n\n'
        fd.write(BinToC.line_wrap(msg, self.max_line, self.indent))

      # Write the index array
      if self.index_list:
        fd.write('const {} {}[] = {{\n'.format(self.index_type, self.index_name))
        indent_str = ' ' * self.indent
        for en in self.entries:
          fd.write(indent_str)
          self._write_index(fd, en)
          fd.write(',\n')
        fd.write('}\n')

    with open(join(out_dir, h_name), 'w') as fd:
      # Write inclue guard and #include stdint
      fd.write('#pragma once\n#include <stdint.h>\n')
      # Include guard for defining the same struct multiple times. This is necessary if the
      # user generates multiple include files for different resources but doesn't change the index type
      fd.write('#ifndef {0}\n#define {0} {0}\n'.format(self.index_type))
      fd.write(BinToC.STRUCT_DECL.format(self.index_type, ' ' * self.indent))
      fd.write('\n#endif\n\n')
      for en in self.entries:
        fd.write('extern const uint8_t {}[{}];\n'.format(en.cname + self.data_suffix, len(en.data)))
        # If we're not creating an index list, then write an individual index entry for each file
        if not self.index_list:
          fd.write('{} {} = '.format(self.index_type, en.cname))
          self._write_index(fd, en)
          fd.write(';\n')
          
      fd.write('\n')
      if self.index_list:
        fd.write('const size_t {}_SIZE = {};\n'.format(self.index_name, len(self.entries)))
        fd.write('extern const {} {}[];\n\n'.format(self.index_type, self.index_name))

  # End generate()

  def _write_index(self, fd, en):
    fd.write('{}(\"{}\", {}, {})'.format(
        self.index_type, en.name, en.cname + self.data_suffix, len(en.data)))
    
  @staticmethod
  def line_wrap(msg, max_len, indent):
    indent_str = ' ' * indent
    out = ''
    while len(msg) > max_len:
      # Go to the end of a line and walk backwards until we find a char we can split on
      i = max_len
      while i >= 0 and BinToC.LINE_WRAP_CHARS.find(msg[i]) == -1:
        i -= 1
      # If the line cannot be split (e.g. VERY long preset name), go forwards until we can split
      if i == -1:
        i = max_len
        while BinToC.LINE_WRAP_CHARS.find(msg[i]) == -1:
          i += 1
      # We found a char to split at, but we want to split AFTER it
      i += 1
      out += msg[0:i] + '\n' + indent_str
      # And move over to the next part of the message
      msg = msg[i:]
    out += msg
    return out

def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('file', nargs='+',
    help='List of input files to process.'
    + ' The filename may contain an = sign, in which case the part before is the file name'
    + ' and the part after will be used as the resource name.')
  parser.add_argument('--length', type=int, default=120,
    help='Maximum line length for wrapping (default: 120)')
  parser.add_argument('-o', '--output', type=str, default='.',
    help='Output directory (default: .)')
  parser.add_argument('-n', '--name', type=str, default='resources',
    help='File names to generate (default: resources)')
  parser.add_argument('-s', '--single', action='store_true', default=False,
    help='Do not generate a list of all included resources')
  parser.add_argument('--prefix', type=str, default='',
    help='Prefix used for C variable names')
  parser.add_argument('--sn', '--struct-name', type=str, default='resource_t',
    help='Name of struct that will hold information about the resource (default: resource_t)')
  args = parser.parse_args(argv)

  btc = BinToC(max_line=args.length, prefix=args.prefix)
  btc.index_type = args.sn
  btc.index_list = not args.single
  for fn in args.file:
    if '=' in fn:
      parts = fn.split('=')
      path = parts[0]
      name = parts[1]
      
    else:
      path = fn
      name = normpath(fn).replace('\\', '/')

    with open(path, "rb") as fd:
      data = fd.read()
    btc.add(name, data)
  btc.generate(args.output, args.name)

if __name__ == '__main__':
  main(sys.argv[1:])
