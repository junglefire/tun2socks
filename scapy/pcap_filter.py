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
@click.option('--pcap', "-p", type=str, required=True, help='pcap file')
@click.option('--filter', "-f", type=str, required=True, help='packet filter')
@click.option('--debug', type=bool, required=False, is_flag=True, help='print debug log information')
def raw_to_socks2(pcap, filter, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 读取pcap文件
	packets = sc.rdpcap(pcap)
	log.info("PCAP file info: {}".format(packets))
	# 配置过滤列表
	[ip_src, ip_dst] = filter.split(",")
	# 读取packet
	count = 0
	for p in packets:
		if (ip_dst == p['IP'].dst and ip_src == p['IP'].src) or (ip_dst == p['IP'].src and ip_src == p['IP'].dst):
		# if (ip_dst == p['IP'].dst and ip_src == p['IP'].src):
			count+=1
			log.info("[{}]{}->{}: {}/{}".format(p.time, p['IP'].src, p['IP'].dst, int(p['Raw'].load[3]), int(p['Raw'].load[4])))
	# summary
	log.info("filter {} packets...".format(count))
	pass

# 读取pcap文件并显示数据包
@click.command()
@click.option('--pcap', "-p", type=str, required=True, help='pcap file')
@click.option('--filter', "-f", type=str, required=True, help='packet filter')
@click.option('--debug', type=bool, required=False, is_flag=True, help='print debug log information')
def raw_to_socks(pcap, filter, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 读取pcap文件
	packets = sc.rdpcap(pcap)
	log.info("PCAP file info: {}".format(packets))
	# 配置过滤列表
	[ip_src, ip_dst] = filter.split(",")
	# 读取packet
	count = 0
	ping = Item()
	pong = Item()
	for p in packets:
		# 请求包
		if (ip_dst == p['IP'].dst and ip_src == p['IP'].src):
			count+=1
			ping.seq = int(p['Raw'].load[3])
			ping.type = int(p['Raw'].load[4])
			ping.time = p.time
			log.debug("PING[{}->{}]: {}".format(p['IP'].src, p['IP'].dst, ping))
			continue
		# 响应包
		elif (ip_dst == p['IP'].src and ip_src == p['IP'].dst):
			count+=1
			pong.seq = int(p['Raw'].load[3])
			pong.type = int(p['Raw'].load[4])
			pong.time = p.time
			log.debug("PONG[{}->{}]: {}".format(p['IP'].src, p['IP'].dst, pong))
		else:
			continue
		# 根据PING包和PONG包计算延时，注意，PING包总是在PONG包之前
		if ping.seq != 0 and pong.seq != 0:
			assert((ping.seq == pong.seq) and (ping.type+1 == pong.type))
			log.info("time: {}".format((pong.time-ping.time)*1000))
	# summary
	log.info("filter {} packets...".format(count))
	pass

# 主函数
if __name__ == '__main__':
	raw_to_socks()








