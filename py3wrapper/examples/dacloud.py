from dacloud import DaCloud
import sys

if len(sys.argv) == 3:
    dc = DaCloud()
    if dc.load_config(sys.argv[1]):
        print(dc.detect({"user-agent":sys.argv[2]}))
