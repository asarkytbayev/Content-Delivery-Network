#
# CS5700 Fall 2019
# Project 5: Roll Your Own CDN
# @author Azamat Sarkytbayev
# NU ID: 001873077
#

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

import json
import sys

ip = sys.argv[1]
api_key = 'at_tfT7wy0qSYMk4TCCvGUuZgkVo9WGT'
api_url = 'https://geo.ipify.org/api/v1?'

url = api_url + 'apiKey=' + api_key + '&ipAddress=' + ip

# print(urlopen(url).read().decode('utf8'))

f = open("lat_lng.txt", "w+")

response = urlopen(url).read().decode('utf8')
if response.find("403") != -1 or response.find("402") != -1:
    f.write("ERROR")
else:
    response_json = json.loads(response)
    f.write(str(response_json["location"]["lat"]) + "\n")
    f.write(str(response_json["location"]["lng"]))
f.close()