#
# CS5700 Fall 2019
# Project 5: Roll Your Own CDN
# @author Azamat Sarkytbayev
# NU ID: 001873077
# run: ./httpserver -p 40006 -o ec2-54-158-114-217.compute-1.amazonaws.com
#

#!/usr/bin/env python3
from http.server import HTTPServer, BaseHTTPRequestHandler
import http.client
from sys import getsizeof, maxsize, argv, exit
import os
from copy import deepcopy
import socket

if len(argv) != 5:
    print("usage: ./httpserver -p <port> -o <origin>")
    exit()


# specifies directory which contains the cached files
cache_dir = "./cache/" 

# creates the directory
if os.path.isdir(cache_dir) == False:
    os.mkdir(cache_dir)

# team 6 port: 40006
port = int(argv[2])
# print("port: {}".format(port))

# origin's ip: "54.158.114.217" 
origin_ip = socket.gethostbyname(argv[4])
# print("origin ip: {}".format(origin_ip))

# http connection with the origin
conn = http.client.HTTPConnection(origin_ip) 

# current cache size in bytes. limit 10Mb - 10_000_000 bytes
cache_size_current = 0

# limit for the cache size
cache_threshold = 9_000_000.0

# hashtable: title -> count - perfect - we do not delete anything from it
cache_lfu_perfect = {} 

cache_flu_present = set()

# hashtable: title -> size in bytes
cache_sizes = {} 

# subclasses BaseHTTPRequestHandler
class HttpServer(BaseHTTPRequestHandler):

    # free given amount of memory, so that new file can be added
    # delete least frequently accessed file(s), however, retain entry(ies) in the hash table
    def delete_lfu(self, memory_to_free: int)->None:
        global cache_size_current
        memory_freed = 0

        while memory_freed < memory_to_free:
            # find least frequently request file
            least_used = ""
            frequency = maxsize
            # for key, value in cache_lfu_perfect.items():
            for key in cache_flu_present:
                value = cache_lfu_perfect[key]
                if value < frequency:
                    least_used = key
                    frequency = value
            # delete the file, remove from hashtable _present and update the current cache size
            os.remove(cache_dir+least_used)
            cache_flu_present.remove(least_used)
            # cache_lfu_perfect.pop(least_used)
            memory_delta = cache_sizes[least_used]
            cache_size_current -= memory_delta
            # update the memory freed variable
            memory_freed += memory_delta

    # implements GET 
    def do_GET(self):
        global cache_size_current
        # ignores /w/index.php?title=
        title = self.path[19:]
        title = title.replace("/", "_")

        try: # checks if file is cached
            file_to_open = open(cache_dir+title, "rb")
            data_to_send = file_to_open.read()
            cache_lfu_perfect[title] += 1
            cache_flu_present.add(title)
            self.send_response(200)
        except IOError: # file not cached - fetches from origin
            conn.request("GET", self.path)
            response = conn.getresponse()
            data_to_send = response.read()
            # print(response.status, response.reason)

            # origin does not contain the file
            if response.status == 404: 
                data_to_send = b"Not Found"
                self.send_response(404)
            elif response.status == 200: # successfully fetched file from the origin                
                # insert size into cache_sizes hashtable
                data_to_send_size = getsizeof(data_to_send)
                
                # checks if there's enough memory to cache the file
                cache_size_new = cache_size_current + data_to_send_size
                memory_to_free = cache_size_new - cache_threshold

                if memory_to_free > 0: # we need to free some memory
                    self.delete_lfu(memory_to_free)

                # cache the file
                file_to_cache = open(cache_dir+title, "wb")
                file_to_cache.write(data_to_send)
                
                cache_sizes[title] = os.stat(cache_dir+title).st_size
                cache_size_current += cache_sizes[title]

                # update the frequency
                if title in cache_lfu_perfect:
                    cache_lfu_perfect[title] += 1
                else:
                    cache_lfu_perfect[title] = 1
                
                cache_flu_present.add(title)
                self.send_response(200)
            
                
        self.end_headers()
        self.wfile.write(data_to_send)

# run the server
httpd = HTTPServer(('', port), HttpServer)
httpd.serve_forever()