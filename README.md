# [Content Delivery Network](https://course.ccs.neu.edu/cs5700f19/project5.html): A network which serves Wikipedia pages efficiently to clients around the world.
Class: [CS5700. Network Fundamentals](http://catalog.northeastern.edu/search/?P=CS%205700)

## Table of Contents:
* [Overview](#overview)
* [DNS Server](#dns)
* [HTTP Server](#httpserver)
* [Discussion](*discussion)


## Overview
The task was to build a Content Delivery Network which would result in fastest download times of Wikipedia pages, using Amazon EC2 hosts for clients around the globe.  

## DNS Server
DNS server runs at cs5700cdnproject.ccs.neu.edu and listen to queries for cs5700cdn.example.com. It directs the clients to the closest HTTP server by distance, by calling ip_config script. External website https://geo.ipify.org/ was used to get the geolocation of the clients and write information to a file, which DNS server would read and perform distance calculations.  

## HTTP Server
HTTP server runs and gets a request for a Wikipedia page and returns it. It uses LFU (least frequently used) algorithm for caching the pages locally, with a 10MB limit on a machine.  

## Results
Top CDN of the class - lowest average download time of Wikipedia pages.  

## Discussion
A better approach for a DNS server would be to use dynamic measurements, like pinging. Geolocation does not directly translate to the shortest distance through the cables or take traffic into account.  
For HTTP servers better approach would be to implement and test  
Perfect-LFU, In-Cache-LFU, LRU and GD-Size. In addition, it's best to precompute the frequency based on the zipf distribution and predownload  files which would result in optimal performance.