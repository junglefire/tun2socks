# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import click
import time

import scapy.all as sc
from packet import *

# 读取pcap文件并显示数据包
@click.command()
@click.option('--pipe', "-p", type=str, required=True, help='named pipe')
@click.option('--source', "-s", type=str, required=True, help='source ip and port')
@click.option('--dest', "-d", type=str, required=True, help='target ip and port')
@click.option('--num-of-request', "-n", type=int, required=True, help='number of request')
@click.option('--debug', type=bool, required=False, is_flag=True, help='print debug log information')
def raw_to_socks(pipe, source, dest, num_of_request, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	pkt = TCPPacket(source, dest, pipe, debug)
	pkt.connect()
	for i in range(0, num_of_request):
		pkt.sendmsg('hello')
	pkt.close()
	pass

# 主函数
if __name__ == '__main__':
	raw_to_socks()








