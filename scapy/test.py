# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import click
import time

import scapy.all as sc
from socket import *

import os, sys
from select import select
from scapy.all import IP

f = os.open("/dev/utun1", os.O_RDWR)
try:
	while 1:
		r = select([f],[],[])[0][0]
		if r == f:
			packet = os.read(f, 4000)
			# print len(packet), packet
			ip = IP(packet)
			ip.show()
except KeyboardInterrupt:
	print("Stopped by user.")




