# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import time

import scapy.all as sc
from socket import *

class TCPPacket:
	# ctor
	# 带入源IP和端口、目标IP和端口
	def __init__(self, source, destination, device='nic'):
		self.source = source
		self.destination = destination
		log.info("create tcp connection from `{}` ->> `{}`".format(self.source, self.destination))
		(self.sip, self.sport) = self.source.split(':')
		(self.dip, self.dport) = self.destination.split(':')
		self.sport = int(self.sport)
		self.dport = int(self.dport)
		# IP包
		self.IP = sc.IP(src=self.sip, dst=self.dip)
		# 初始的seq和ack
		self.seq = 0
		self.ack = 0
		# 通过网卡发送或者通过命名管道发送
		if device == "nic":
			self.nic = True
		else:
			self.nic = False
		pass

	# TCP三次握手
	def connect(self):
		SYN = self.IP/sc.TCP(sport=self.sport, dport=self.dport, flags='SEC', seq=self.seq, ack=self.ack)
		log.debug("{}".format(repr(SYN)))
		SYNACK = self.__send(SYN)
		log.debug("{}".format(repr(SYNACK)))
		ACK=self.IP/sc.TCP(sport=self.sport, dport=self.dport, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
		log.debug("{}".format(repr(ACK)))
		self.__send_without_answer(ACK)
		self.seq = ACK.seq
		self.ack = ACK.ack
		pass

	# 发送数据包
	def sendmsg(self, data):
		PUSH = self.IP/sc.TCP(sport=self.sport, dport=self.dport, flags="PA", seq=self.seq, ack=self.ack)/sc.Raw(data)
		log.debug("{}".format(repr(PUSH)))
		PUSHACK = self.__send(PUSH)
		log.debug("{}".format(repr(PUSHACK)))
		self.seq = PUSHACK.ack
		self.ack = PUSHACK.seq+PUSHACK.len-40
		time.sleep(5)

	# 端口连接
	def close(self):
		FIN = self.IP/sc.TCP(sport=self.sport, dport=self.dport, flags="FA", seq=self.seq, ack=self.ack)
		log.debug("{}".format(repr(FIN)))
		FINACK = self.__send(FIN)
		log.debug("{}".format(repr(FINACK)))
		LASTACK = self.IP/sc.TCP(sport=self.sport, dport=self.dport, flags="A", seq=FINACK.ack, ack=FINACK.seq+1)
		self.__send_without_answer(LASTACK)
		time.sleep(10)

	# 发送函数，可以通过socket发送、也可以写入到命名管道
	def __send(self, packet):
		answer = sc.sr1(packet)
		return answer

	# 发送函数，可以通过socket发送、也可以写入到命名管道，不需要等待回包
	def __send_without_answer(self, packet):
		sc.sr1(packet)








