from ctypes import *
lib=cdll.LoadLibrary('./libclsPyVariableChunk.so')
lib.ProcessFileToVar.restype=c_char_p
result=lib.ProcessFileToVar('/Users/jwang/bbb_sunflower_1080_2min.mp4', 0, 2, 64, 0, True, True, '/tmp/slo/', True)
print result

