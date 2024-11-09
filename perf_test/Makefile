
.PHONY: perf_top perf_stat

OUT_PREFIX := UDP
STAT_CMD := iperf3 -c 192.168.116.130 -B 192.168.116.128 -u -l 1460 -t 30 -i 5 -b 10G -P 1
FLAME_CMD := iperf3 -c 192.168.116.130 -B 192.168.116.128 -u -l 1460 -t 30 -i 5 -b 10G -P 1.0

perf_top:
	@sudo perf top \
	-c 200000 \
	--call-graph fractal \
	-n \
	-d 2 \
	> perf_top.log


perf_stat:
	@sudo perf stat \
	-o $(OUT_PREFIX).log \
	-d -d -d \
	-- \
	$(STAT_CMD)

perf_flame:
	@sudo perf record -F 10000 -g -- $(FLAME_CMD)
	@sudo perf script -i perf.data > out.perf 
	@./FlameGraph/stackcollapse-perf.pl out.perf > out.folded
	@./FlameGraph/flamegraph.pl out.folded > $(OUT_PREFIX).svg
	@cp $(OUT_PREFIX).svg /mnt/hgfs/share

clean:
	@sudo rm -f out* perf.data *.svg *.log

