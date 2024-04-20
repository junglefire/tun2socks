# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import click
import time

from socket import *
from packet import *

# 通过管道发送UDP包
@click.command()
@click.option('--pipe', '-p', type=str, required=True, help='named pipe')
@click.option('--source', '-s', type=str, required=True, help='source ip and port')
@click.option('--destination', '-d', type=str, required=True, help='destination ip and port')
@click.option('--debug', type=bool, required=False, is_flag=True, help='print debug log information')
def main_func(pipe, source, destination, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	tp = TCPPacket(source, destination)
	tp.connect()
	tp.sendmsg("hello")
	tp.close()
	pass


# 主函数
if __name__ == '__main__':
	main_func()








