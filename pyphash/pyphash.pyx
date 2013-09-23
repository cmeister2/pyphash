#
# Cython wrapper code for pHash
#
# By Mostafa Razavi
#

# import malloc, free, etc.
from libc.stdlib cimport *

cdef extern from "pHash.h":
  ctypedef unsigned long long ulong64
  cdef struct ph_hashtype:
    char* id
    void* hash
    float* path
    int hash_length
    short int hash_type
  ctypedef ph_hashtype DP
  
  struct doubleArray:
    double *array
    int used
    int size
    
  struct ulong64Array:
    ulong64 *array
    int used
    int size
    
  int ph_dct_imagehash(char* file, ulong64 hash)
  ulong64* ph_dct_videohash(char* filename, int &Length)
  int ph_hamming_distance(ulong64 hasha, ulong64 hashb)
  DP** ph_dct_image_hashes(char* files[], int count, int threads)
  DP** ph_dct_video_hashes(char* files[], int count, int threads)

cdef extern from "videoinfo.h":
  cdef struct ph_hashdata:
    doubleArray pts
    ulong64Array hashes
    int keyframes
  ctypedef ph_hashdata HASHDATA  

  HASHDATA *ph_max_videohash(char *file)

class Hash(object):
  def __init__(self, id, hash):
    self.id = id
    if type(hash) == list:
      self.hash = hash
    else:
      self.hash = [hash]

  def __repr__(self):
    return "<Hash('{0}')={1}>".format(self.id, self.hash[0] if len(self.hash) == 1 else self.hash)

class PHashException(Exception):
  pass

def max_videoHash(file):
  cdef HASHDATA* hdata
  print "Getting hash data for {0}".format(file)
  
  hdata = ph_max_videohash(file)

  ret = []
  for i in xrange(0, hdata.keyframes):
    tup = (hdata.hashes.array[i], hdata.pts.array[i])
    ret.append(tup)
    
  return ret

def imageHash(file):
  cdef ulong64 hash
  ret = ph_dct_imagehash(file, hash)
  if ret == -1:
    raise PHashException()

  return hash

def videoHash(file):
  cdef ulong64* hash
  cdef int length
  cdef int i

  print "Attempting to get hash"

  hash = ph_dct_videohash(file, length)
  
  if hash == NULL:
    print "Oh dear"
    raise PHashException()
  
  print "Got hash"

  ret = []
  for i in xrange(0, length):
    ret.append(hash[i])

  free(hash)

  return ret

def hammingDistance(ulong64 hash1, ulong64 hash2):
  return ph_hamming_distance(hash1, hash2)

def imageHashes(files, threads):
  if threads < 1 or threads > len(files):
    threads = len(files)

  # first convert files into a C array
  cdef char** cfiles = <char**> malloc(len(files) * sizeof(char*))
  cdef int i

  for i in xrange(0, len(files)):
    cfiles[i] = files[i]

  # now call the wrapped function and convert its return values to Python objects
  cdef DP** ret = ph_dct_image_hashes(cfiles, len(files), threads)

  hashes = []
  for i in xrange(0, len(files)):
    # [0] dereferences ret[i].hash which is a pointer
    hashes.append(Hash(ret[i].id, (<ulong64*> ret[i].hash)[0]))

  # free allocated array and the returned values
  free(cfiles)
  for i in xrange(0, len(files)):
    free(ret[i].id)
    free(ret[i].hash)
    free(ret[i])
  free(ret)

  return hashes

def videoHashes(files, threads):
  if threads < 1 or threads > len(files):
    threads = len(files)

  # first convert files into a C array
  cdef char** cfiles = <char**> malloc(len(files) * sizeof(char*))
  cdef int i
  cdef int j

  for i in xrange(0, len(files)):
    cfiles[i] = files[i]

  # now call the wrapped function and convert its return values to Python objects
  cdef DP** ret = ph_dct_video_hashes(cfiles, len(files), threads)

  hashes = []
  for i in xrange(0, len(files)):
    h = []
    for j in xrange(0, ret[i].hash_length):
      h.append((<ulong64*> ret[i].hash)[j])
    hashes.append(Hash(ret[i].id, h))

  # free allocated array and the returned values
  free(cfiles)
  for i in xrange(0, len(files)):
    free(ret[i].id)
    free(ret[i].hash)
    free(ret[i])
  free(ret)

  return hashes