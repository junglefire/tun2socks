# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import click
import time

import scapy.all as sc
from socket import *

# 读取pcap文件并显示数据包
@click.command()
@click.option('--raw', type=str, required=True, help='raw packet file')
@click.option('--pcap', type=str, required=True, help='pcap file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def raw_to_pcap(raw, pcap, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 读取packet
	packets = []
	with open(raw, 'rb') as f:
		while (True):
			buffer = f.read(4)
			if len(buffer) <= 0:
				break
			length = int.from_bytes(buffer, "little")
			log.info("PACKET LEN: %d", length)
			buffer = f.read(length)
			PACKET = sc.IP(_pkt=buffer)
			# PACKET.show()
			packets.append(PACKET)
	sc.wrpcap(pcap, packets)
	pass

# 主函数
if __name__ == '__main__':
	raw_to_pcap()








