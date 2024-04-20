# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import click
import time

import scapy.all as sc
from socket import *

# 源地址和目标地址
SOURCE_IP   = "30.211.112.115"
SOURCE_PORT = 4444
TARGET_IP 	= "47.88.22.87"
TARGET_PORT = 19765

# PCAP文件中的源地址和目标地址
PCAP_SOURCE_IP   = "10.25.0.1"
PCAP_SOURCE_PORT = 4444
PCAP_TARGET_IP 	= "172.168.0.1"
PCAP_TARGET_PORT = 19765

# 通过管道发送UDP包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def test_udp_packet(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	pkt = sc.IP(src=SOURCE_IP, dst=TARGET_IP)/sc.UDP(sport=SOURCE_PORT, dport=TARGET_PORT)/sc.Raw(load="alex")
	while True:
		client.send(sc.raw(pkt))
		time.sleep(3)
		data = client.recv(1024)
		# 略过Null/Loopback标志[前4个字节]
		recv_pkt = sc.IP(_pkt=data[4:])
		recv_pkt.show()
		# log.info("{}".format(data.hex()))
	pass


# 通过管道发送TCP包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def test_tcp_packet(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# 发送SYN包
	syn_pkt = sc.IP(src=SOURCE_IP, dst=TARGET_IP)/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='S',seq=0)
	client.send(sc.raw(syn_pkt))
	# 接收SYN ACK包
	syn_ack_data = client.recv(1024)
	syn_ack_pkt = sc.IP(_pkt=syn_ack_data[4:])
	syn_ack_pkt.show()
	# 发送ACK包
	seq = syn_ack_pkt.seq + 1
	ack = syn_ack_pkt.ack
	ack_pkt = sc.IP(src=SOURCE_IP, dst=TARGET_IP)/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='A', seq=ack, ack=seq)
	client.send(sc.raw(ack_pkt))
	time.sleep(10)
	# 发送DATA包
	seq = ack_pkt.seq + 1
	ack = ack_pkt.ack
	log.info("send data packet...")
	data_pkt = sc.IP(src=SOURCE_IP, dst=TARGET_IP)/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='', seq=ack, ack=seq)/sc.Raw("alex")
	client.send(sc.raw(data_pkt))
	log.info("recv data ack packet...")
	# 接收响应
	data = client.recv(1024)
	recv_pkt = sc.IP(_pkt=data[4:])
	recv_pkt.show()
	pass


# 直接发送TCP包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def test_tcp_packet2(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# SYN
	ip = sc.IP(dst=TARGET_IP)
	SYN = sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='S', seq=1, ack=0)
	SYNACK=sc.sr1(ip/SYN)
	SYNACK.show()
	# ACK
	ACK=sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
	sc.send(ip/ACK)
	# 发送DATA包
	log.info("send data packet...")
	PUSH = TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="PA", seq=SYNACK.ack, ack=SYNACK.seq+1)
	PUSHACK = sr1(ip/PUSH/b"\x41\x0a")
	time.sleep(5)
	FIN = TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="FA", seq=PUSHACK.ack, ack=PUSHACK.seq)
	FINACK = sr1(ip/FIN)
	LASTACK = TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="A", seq=FINACK.ack, ack=FINACK.seq + 1)
	send(ip/LASTACK)
	pass

# 通过管道发送TCP包，不包含数据包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def test_tcp_packet4(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# SYN
	IP = sc.IP(src=SOURCE_IP, dst=TARGET_IP)
	SYN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='S', seq=1, ack=0)
	client.send(sc.raw(SYN))
	SYNACK = sc.IP(_pkt=client.recv(1024)[4:])
	SYNACK.show()
	# ACK
	ACK=IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
	client.send(sc.raw(ACK))
	# 发送FIN包
	log.info("send fin packet...")
	FIN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="FA", seq=ACK.ack, ack=ACK.seq)
	client.send(sc.raw(FIN))
	FINACK = sc.IP(_pkt=client.recv(1024)[4:])
	FINACK.show()
	log.info("send last ack packet...")
	LASTACK = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="A", seq=FINACK.ack, ack=FINACK.seq+1)
	client.send(sc.raw(LASTACK))
	time.sleep(10)
	pass


# 读取pcap文件并显示数据包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--pcap', type=str, required=True, help='pcap file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
@click.option('--show', type=str, required=False, default='show', help='<show|repr|byte>')
def test_load_pcap(pipe, pcap, show, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# 读取pcap文件
	packets = sc.rdpcap(pcap)
	log.info("PCAP file info: {}".format(packets))
	# 读取packet
	for p in packets:
		if show == 'show':
			p.show()
		elif show == 'repr':
			log.info("{}".format(repr(p)))
		else:
			log.info("{}".format(p))
	pass

# 读取pcap文件并发送数据包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--pcap', type=str, required=True, help='pcap file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def test_send_pcap(pipe, pcap, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# 读取pcap文件
	packets = sc.rdpcap(pcap)
	log.info("PCAP file info: {}".format(packets))
	# 读取packet
	for p in packets:
		print(type(p['IP']))
		if p['IP'].src == PCAP_SOURCE_IP:
			log.info("send: {}".format(repr(p['IP'])))
			log.info("send: {}".format(bytes(p).hex()))
			client.send(bytes(p['IP']))
		else:
			ack = sc.IP(_pkt=client.recv(1024)[4:])
			log.info("recv: {}".format(repr(ack)))
	pass


# 通过管道发送TCP包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def send_tcp_packet_by_pipe(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# SYN
	IP = sc.IP(src=SOURCE_IP, dst=TARGET_IP)
	SYN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='SEC', seq=1, ack=0)
	client.send(sc.raw(SYN))
	SYNACK = sc.IP(_pkt=client.recv(1024)[4:])
	SYNACK.show()
	# ACK
	ACK=IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
	client.send(sc.raw(ACK))
	# 发送DATA包`hello`
	log.info("send data packet `hello`...")
	PUSH = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="PA", seq=ACK.seq, ack=ACK.ack)/sc.Raw("hello")
	client.send(sc.raw(PUSH))
	log.info("recv data packet ack...")
	PUSHACK = sc.IP(_pkt=client.recv(1024)[4:])
	PUSHACK.show()
	time.sleep(5)
	# 发送DATA包`world`
	log.info("send data packet `world`...")
	PUSH = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="PA", seq=PUSHACK.ack, ack=PUSHACK.seq+PUSHACK.len-40)/sc.Raw("world")
	client.send(sc.raw(PUSH))
	log.info("recv data packet ack...")
	PUSHACK = sc.IP(_pkt=client.recv(1024)[4:])
	PUSHACK.show()
	# 发送FIN包
	log.info("send fin packet...")
	FIN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="FA", seq=PUSHACK.ack, ack=PUSHACK.seq+PUSHACK.len-40)
	client.send(sc.raw(FIN))
	FINACK = sc.IP(_pkt=client.recv(1024)[4:])
	FINACK.show()
	LASTACK = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="A", seq=FINACK.ack, ack=FINACK.seq+1)
	client.send(sc.raw(LASTACK))
	time.sleep(10)
	pass


# 通过管道发送TCP包
@click.command()
@click.option('--pipe', type=str, required=True, help='named pipe')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def send_tcp_packet_by_socket(pipe, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 创建Unix套接字
	client = socket(AF_UNIX, SOCK_STREAM)
	client.connect(pipe)
	# SYN
	IP = sc.IP(src=SOURCE_IP, dst=TARGET_IP)
	SYN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='SEC', seq=1, ack=0)
	SYNACK = sc.sr1(SYN)
	SYNACK.show()
	# ACK
	ACK=IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
	sc.send(ACK)
	# 发送DATA包`hello`
	log.info("send data packet `hello`...")
	PUSH = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="PA", seq=ACK.seq, ack=ACK.ack)/sc.Raw("hello")
	log.info("recv data packet ack...")
	PUSHACK = sc.sr1(PUSH)
	PUSHACK.show()
	time.sleep(5)
	# 发送DATA包`world`
	log.info("send data packet `world`...")
	PUSH = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="PA", seq=PUSHACK.ack, ack=PUSHACK.seq+PUSHACK.len-40)/sc.Raw("world")
	log.info("recv data packet ack...")
	PUSHACK = sc.sr1(PUSH)
	PUSHACK.show()
	# 发送FIN包
	log.info("send fin packet...")
	FIN = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="FA", seq=PUSHACK.ack, ack=PUSHACK.seq+PUSHACK.len-40)
	FINACK = sc.sr1(FIN)
	FINACK.show()
	LASTACK = IP/sc.TCP(sport=SOURCE_PORT, dport=TARGET_PORT, flags="A", seq=FINACK.ack, ack=FINACK.seq+1)
	sc.send(LASTACK)
	time.sleep(10)
	pass


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
			print(length)
			buffer = f.read(length)
			PACKET = sc.IP(_pkt=buffer)
			# PACKET.show()
			packets.append(PACKET)
	sc.wrpcap("temp.pcap", packets)
	pass

# 主函数
if __name__ == '__main__':
	# test_udp_packet()
	# test_tcp_packet3()
	# test_tcp_packet4()
	# test_send_pcap()
	send_tcp_packet_by_pipe()
	# send_tcp_packet_by_socket()
	# raw_to_pcap()








