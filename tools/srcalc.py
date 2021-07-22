#!/usr/bin/env python

def calc_sr(clk_in, plln, pllr, i2sdiv, odd):
	if clk_in * plln > 432e6 or clk_in * plln < 100e6:
		return False

	if (clk_in * (plln/pllr)) > 192e6:
		return False

	return (clk_in * (plln/pllr)) / ((32*2)*(2*i2sdiv+odd)*4)


rates = []
targets = [44100, 48000]
clk_in = 2e6

for plln in range(50, 433):
	for pllr in range(2, 8):
		for i2sdiv in range(2, 128):
			for odd in [0, 1]:
				sr = calc_sr(clk_in, plln, pllr, i2sdiv, odd)

				if sr == False or sr < 40e3 or sr > 50e3:
					continue

				diff = 1
				for target in targets:
					newdiff = abs(sr-target)/target*100.0
					if newdiff < diff:
						diff = newdiff

				if (int(sr) == sr or diff < 0.05) and rates.count(int(sr)) == 0:
					rates.append(int(sr))
					print(f"{sr}: plln: {plln}, pllr:{pllr}, i2sdiv:{i2sdiv}, odd:{odd}")
