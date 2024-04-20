# -*- coding: utf-8 -*- 
#!/usr/bin/env python
import logging as log
import seaborn as sns
import scapy as sc
import numpy as np
import click
import time
import re

# epoll log miner
@click.command()
@click.option('--logfile', type=str, required=True, help='log file name')
@click.option('--debug', type=bool, required=False, default=False, help='print debug log information')
def logminer(logfile, debug):
	if debug:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.DEBUG)
	else:
		log.basicConfig(format='[%(asctime)s][%(levelname)s] %(message)s', level=log.INFO)
	pass


# 主函数
if __name__ == '__main__':
	logminer()








