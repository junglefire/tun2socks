# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import scapy.all as sc
import logging as log
from socket import *
import time

class Item:
	def __init__(self):
		self.seq = 0
		self.type = 0
		self.time = 0.0

	def __str__(self):
		return 'ts={}, seq={}, type={}'.format(self.time, self.seq, self.type)

# TCP Packet
class TCPPacket:
	# ctor
	# 带入源IP和端口、目标IP和端口
	def __init__(self, source, destination, device, debug=False):
		self.debug = debug
		self.source = source
		self.destination = destination
		log.info("create tcp connection from `{}` to `{}`".format(self.source, self.destination))
		(self.sip, self.sport) = self.source.split(':')
		(self.dip, self.dport) = self.destination.split(':')
		self.sport = int(self.sport)
		self.dport = int(self.dport)
		# IP包
		self.IP = sc.IP(src=self.sip, dst=self.dip)
		# 初始的seq和ack
		self.seq = 1
		self.ack = 0
		self.length = 0
		# 创建管道
		# 创建Unix套接字
		self.client = socket(AF_UNIX, SOCK_STREAM)
		self.client.connect(device)
		pass

	# TCP三次握手
	def connect(self):
		# SYN
		IP = sc.IP(src=self.sip, dst=self.dip)
		SYN = IP/sc.TCP(sport=self.sport, dport=self.dport, flags='SEC', seq=self.seq, ack=self.ack)
		if self.debug:
			SYN.show()
		self.client.send(sc.raw(SYN))
		SYNACK = sc.IP(_pkt=self.client.recv(1024)[4:])
		if self.debug:
			SYNACK.show()
		# ACK
		self.ack = SYNACK.ack
		self.seq = SYNACK.seq
		ACK = IP/sc.TCP(sport=self.sport, dport=self.dport, flags='A', seq=self.ack, ack=self.seq+1)
		if self.debug:
			ACK.show()
		self.client.send(sc.raw(ACK))
		self.ack = ACK.ack
		self.seq = ACK.seq
		pass

	# 发送数据包
	def sendmsg(self, data):
		log.info("send data packet, message `%s`...", data)
		IP = sc.IP(src=self.sip, dst=self.dip)
		if self.length == 0:
			PUSH = IP/sc.TCP(sport=self.sport, dport=self.dport, flags="PA", seq=self.seq, ack=self.ack)/sc.Raw(data)
		else:
			PUSH = IP/sc.TCP(sport=self.sport, dport=self.dport, flags="PA", seq=self.ack, ack=self.seq+self.length-40)/sc.Raw(data)
		if self.debug:
			PUSH.show()
		self.client.send(sc.raw(PUSH))
		log.info("recv data packet ack...")
		PUSHACK = sc.IP(_pkt=self.client.recv(1024)[4:])
		self.ack = PUSHACK.ack
		self.seq = PUSHACK.seq
		self.length = PUSHACK.len
		if self.debug:
			PUSHACK.show()
		pass

	# 端口连接
	def close(self):
		IP = sc.IP(src=self.sip, dst=self.dip)
		# 发送FIN包
		log.info("send fin packet...")
		if self.length != 0:
			FIN = IP/sc.TCP(sport=self.sport, dport=self.dport, flags="FA", seq=self.ack, ack=self.seq+self.length-40)
		else:
			FIN = IP/sc.TCP(sport=self.sport, dport=self.dport, flags="FA", seq=self.ack, ack=self.seq)
		if self.debug:
			FIN.show()
		self.client.send(sc.raw(FIN))
		FINACK = sc.IP(_pkt=self.client.recv(1024)[4:])
		self.ack = FINACK.ack
		self.seq = FINACK.seq
		if self.debug:
			FINACK.show()
		LASTACK = IP/sc.TCP(sport=self.sport, dport=self.dport, flags="A", seq=self.ack, ack=self.seq+1)
		if self.debug:
			LASTACK.show()
		self.client.send(sc.raw(LASTACK))
		time.sleep(5)








