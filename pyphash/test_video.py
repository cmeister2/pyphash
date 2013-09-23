'''
Created on 4 Mar 2013

@author: minty
'''
import pyphash as pyh
import time
import os
import pickle
import pprint

def get_hashpts(filename):
  pkl = '.'.join([filename, "pkl"])
  
  if os.path.exists(pkl):
    with open(pkl, "r") as p:
      hashpts = pickle.load(p)
      
  else:
    hashpts = pyh.max_videoHash(filename)
    with open(pkl, "w") as p:
      pickle.dump(hashpts, p)
  
  return hashpts
    
if __name__ == '__main__':
  
  start_time = time.time()
  
  x = get_hashpts("gangnamstyle.mp4")
  y = get_hashpts("gangnamstyle2.mp4")

  end_time = time.time()
  print("Elapsed time was %g seconds" % (end_time - start_time))

  for index, tup in enumerate(x):
    hash1 = tup[0]
    pts1 = tup[1]
    hash2 = y[index][0]
    pts2 = y[index][1]
    
    ptsdiff = abs(pts1-pts2)
    
    print "Index {0}  Ptsdiff {6}  Hamming Distance {3}".format(index, 
                                                                hash1, 
                                                                hash2, 
                                                                pyh.hammingDistance(hash1, hash2),
                                                                pts1,
                                                                pts2,
                                                                ptsdiff)