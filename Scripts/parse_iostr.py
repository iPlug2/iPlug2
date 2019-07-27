from parse_config import parse_config
import sys, pprint

def main():
  input=sys.argv[1]
  mode=sys.argv[2]

  config = parse_config(input)
  iostr = config['PLUG_CHANNEL_IO']

  iotokens = iostr.split()

  maxninputs = 0
  maxnoutputs = 0

  for io in iotokens:
    ninputs, noutputs = io.split('-')
    if int(ninputs) > maxninputs:
      maxninputs = int(ninputs)
    if int(noutputs) > maxnoutputs:
      maxnoutputs = int(noutputs)

  if mode == "inputs":
    print(maxninputs)
  elif mode == "outputs":
    print(maxnoutputs)

if __name__ == '__main__':
  main()