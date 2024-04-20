# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import scapy as sc
import click
import time
import re

# 通过管道发送TCP包
@click.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--delay', type=int, required=True, help='max delay time')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def check_logfile1(logfile, delay, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# compile pattern
	pattern0 = re.compile(r'E\((\d+)\)\((\d+)\)\((\d+)\)>><<')
	last_time = 0
	lineno = 0;
	# open the log file
	log.info("open log file `{}`...".format(logfile))
	with open(logfile, "r") as reader:
		for line in reader:
			lineno += 1
			m = pattern0.match(line)
			if m == None:
				continue
			timeout = m.group(2)
			if last_time == 0:
				last_time = int(m.group(3))
				continue
			gap = int(m.group(3))-last_time
			log.debug("GAP: {}".format(gap))
			last_time = int(m.group(3))
			if gap > int(delay):
				log.info("BREAK on line: {}, GAP({})".format(lineno, gap))
	pass


# 通过管道发送TCP包
@click.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--tid', type=str, required=False, help='thread id')
@click.option('--delay', type=int, required=True, help='max delay time')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def check_logfile2(logfile, tid, delay, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# 状态转换图
	states = ["TCB", "PCB", "IOCB", "CKCB", "CLCB"]
	num_of_status = 5
	last_time = 0
	last_line = ""
	lineno = 0
	idx = 0
	# open the log file
	log.info("open log file `{}`...".format(logfile))
	with open(logfile, "r") as reader:
		for line in reader:
			lineno += 1
			if tid and line.startswith(tid) == False:
				continue
			line = line.replace('\n', '')
			items = line.split(":")
			if last_time == 0:
				last_time = int(items[1])
				last_line = line
			else:
				gap = int(items[1])-last_time
				if gap > int(delay):
					stime = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(items[1])/1000))
					log.info("+ BREAK on line: {}, time: {}, event: {}, GAP({})".format(lineno, stime, items[2], gap))
					log.info("  - BREAK on line: {}, {}".format(lineno-1, last_line))
					log.info("  - BREAK on line: {}, {}".format(lineno, line))
				last_time = int(items[1])
				last_line = line
	pass


# 主函数
if __name__ == '__main__':
	check_logfile2()








