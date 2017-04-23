from dacloud import DaCloud
import sys
import time

if len(sys.argv) == 3:
    dc = DaCloud()
    props= {}
    start = end = 0
    if dc.load_config(sys.argv[1]):
        print("config loaded\n")
        start = time.time()
        for x in range(0, 10000):
            props = dc.detect({"user-agent":sys.argv[2]})
        end = time.time()

    print(props)
    print("Time spent for 10000 detections %ld sec\n" % (end - start))
    print("the cache id is %s\n" % dc.cache_id())
