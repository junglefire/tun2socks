# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import matplotlib.pyplot as plt
import matplotlib as mpl
import logging as log
import seaborn as sns
import pandas as pd
import numpy as np
import click
import time
import re
import os

plt.rcParams['font.family'] = ['Arial Unicode MS'] #用来正常显示中文标签
plt.rcParams['axes.unicode_minus'] = False #用来正常显示负号

sns.set_style('whitegrid',{'font.sans-serif':['Arial Unicode MS','Arial']})


# main loop
@click.group()
def main_func():
	pass

# epoll log miner
@main_func.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def logminer(logfile, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	last_time = -1
	timeout = 0
	lineno = 0
	# temp dict
	td = {"lineno": [], "asctime": [], "timeout": [], "execute_time": []}
	# event regex pattern
	pattern0 = re.compile(r'(\d+):(\d+):\[(\d+)\]:epoll_wait>')
	pattern1 = re.compile(r'(\d+):(\d+):\[(\d+)\]:<epoll_wait')
	# load epoll log 
	with open(logfile, "r") as reader:
		for line in reader:
			lineno += 1
			m0 = pattern0.match(line)
			if m0:
				timeout = m0.group(3)
				last_time = m0.group(2)
				continue
			else:
				m1 = pattern1.match(line)
				if m1:
					if last_time == -1:
						continue
					gap = int(m1.group(2)) - int(last_time)
					last_time = m1.group(2)
					strtime = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(m1.group(2))))
					td['lineno'].append(lineno)
					td['asctime'].append(strtime)
					td['timeout'].append(int(timeout))
					td['execute_time'].append(int(gap))
	# create dataframe from dict
	df = pd.DataFrame(td)
	# df["asctime"] = pd.to_datetime(df["asctime"], format="%Y-%m-%d %H:%M:%S")
	# df1 = df.groupby("asctime").mean()
	print(df.head(100))
	# lineplot
	fig, ax = plt.subplots(figsize = (15, 10))
	ax.set_ylabel('time(ms)')
	# sns.lineplot(x="asctime", y="execute_time", data=df)
	sns.lineplot(x=df.index, y="execute_time", data=df, label="epoll_wait执行时间")
	sns.lineplot(x=df.index, y="timeout", data=df, label="epoll_wait超时时间")
	plt.show()


# split epoll log
@main_func.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--opath', type=str, required=True, help='output path')
@click.option('--event', type=str, required=False, help='event filter')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def split(logfile, opath, event, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# file pool
	files = {}
	lineno = 0
	# load epoll log 
	with open(logfile, "r") as reader:
		for line in reader:
			lineno+=1
			if event and event not in line:
				continue
			items = line.split(":")
			tid = items[0]
			if tid not in files:
				log.info("T: {}, L: {}".format(tid, line[:-1]))
				files[tid] = open(opath+"/"+tid+".log", "w+")
			files[tid].write("{}:{}".format(lineno, line))
	# close all files
	for _, file in files.items():
		file.close()


# format one epoll log
@main_func.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--ofile', type=str, required=True, help='output file name')
@click.option('--event', type=str, required=True, help='event filter')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def format(logfile, ofile, event, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	__format(logfile, ofile, event)
	pass


# format some log files
@main_func.command()
@click.option('--dir', type=str, required=True, help='log files directory')
@click.option('--ofile', type=str, required=True, help='output file name')
@click.option('--event', type=str, required=True, help='event filter')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def formatex(dir, ofile, event, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# open output file
	try:
		os.remove(ofile)
	except FileNotFoundError:
		log.info("{} not exist!".format(ofile))
	# traverse directory
	for root, dirs, files in os.walk(dir, topdown=True):
		for logfile in sorted(files):
			__format("{}/{}".format(dir, logfile), ofile, event)
	pass


def __format(logfile, ofile, event):
	log.info("format file: `{}`...".format(logfile))
	# status
	last_time = -1
	timeout = 0
	lineno = 0
	lineno_begin = 0
	# event regex pattern
	re0 = r"(\d+):(\d+):(\d+):\[(\d+)\]:{}>".format(event)
	re1 = r"(\d+):(\d+):(\d+):\[(\d+)\]:<{}".format(event)
	pattern0 = re.compile(re0)
	pattern1 = re.compile(re1)
	# open output file
	fout = open(ofile, "a+")
	if os.path.getsize(ofile) == 0:
		fout.write("{},{},{},{},{},{},{},{}\n".format("thread_id", "time>", "time<", "lineno>", "lineno<", "timeout", "nfds", "gap"))
	# load epoll log 
	with open(logfile, "r") as reader:
		for line in reader:
			if event not in line:
				continue
			m0 = pattern0.match(line)
			if m0:
				timeout = m0.group(4)
				last_time = m0.group(3)
				lineno_begin = m0.group(1)
				continue
			else:
				m1 = pattern1.match(line)
				if m1:
					if last_time==-1:
						log.error("INVALID: `{}`".format(line[:-1]))
						continue
					gap = int(m1.group(3)) - int(last_time)
					time_begin = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(last_time)/1000))
					time_end = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(m1.group(3))/1000))
					# lineno, thread id, time>, time<, lineno>, lineno<, timeout, nfds, execute time
					fout.write("{},{},{},{},{},{},{},{}\n".format(m1.group(2), time_begin, time_end, lineno_begin, m1.group(1), timeout, m1.group(4), gap))
	fout.close()
	pass


def __format2(logfile, ofile, event):
	log.info("format file: `{}`...".format(logfile))
	# status
	last_time = -1
	timeout = 0
	lineno = 0
	lineno_begin = 0
	# event regex pattern
	re0 = r"(\d+):(\d+):(\d+):{}>".format(event)
	re1 = r"(\d+):(\d+):(\d+):<{}".format(event)
	pattern0 = re.compile(re0)
	pattern1 = re.compile(re1)
	# open output file
	fout = open(ofile, "a+")
	if os.path.getsize(ofile) == 0:
		fout.write("{},{},{},{},{},{},{},{}\n".format("thread_id", "time>", "time<", "lineno>", "lineno<", "timeout", "nfds", "gap"))
	# load epoll log 
	with open(logfile, "r") as reader:
		for line in reader:
			if event not in line:
				continue
			m0 = pattern0.match(line)
			if m0:
				last_time = m0.group(3)
				lineno_begin = m0.group(1)
				continue
			else:
				m1 = pattern1.match(line)
				if m1:
					if last_time==-1:
						log.error("INVALID: `{}`".format(line[:-1]))
						continue
					gap = int(m1.group(3)) - int(last_time)
					time_begin = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(last_time)/1000))
					time_end = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(m1.group(3))/1000))
					# lineno, thread id, time>, time<, lineno>, lineno<, timeout, nfds, execute time
					fout.write("{},{},{},{},{},{},{},{}\n".format(m1.group(2), time_begin, time_end, lineno_begin, m1.group(1), timeout, 0, gap))
	fout.close()
	pass


# check epoll_wait events
@main_func.command()
@click.option('--csvfile', type=str, required=True, help='csv file name')
@click.option('--delay', type=int, required=True, help='gap')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def check(csvfile, delay, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# load csv file
	df = pd.read_csv(csvfile, sep=',')
	df = df[df['gap']>delay]
	print(df)


# show epoll_wait events lineplot
@main_func.command()
@click.option('--csvfile', type=str, required=True, help='csv file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def lineplot(csvfile, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	# load csv file
	df = pd.read_csv(csvfile, sep=',')
	# lineplot
	fig, ax = plt.subplots(figsize = (15, 10))
	ax.set_ylabel('time(ms)')
	df["time>"] = pd.to_datetime(df["time>"], format="%Y-%m-%d %H:%M:%S")
	log.info("convert data type ...")
	sns.lineplot(x="time>", y="gap", data=df)
	sns.lineplot(x="time>", y="timeout", data=df)
	# sns.lineplot(x=df.index, y="gap", data=df, label="epoll_wait执行时间")
	# sns.lineplot(x=df.index, y="timeout", data=df, label="epoll_wait超时时间")
	log.info("drawing lineplot ...")
	plt.show()
	

# 主函数
if __name__ == '__main__':
	main_func()








