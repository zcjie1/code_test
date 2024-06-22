
.PHONY: perf_top perf_stat

perf_top:
	@sudo perf top \
	-c 200000 \
	--call-graph fractal \
	-n \
	-d 2 \
	> perf_top.log


perf_stat:
	@sudo perf stat \
	-o perf_stat.log \
	-d -d -d \
	-- \
	tree /home/zcj

perf_flame:
	@sudo perf record -F 10000 -g -- ../c_test/test
	@sudo perf script -i perf.data > out.perf 
	@./FlameGraph/stackcollapse-perf.pl out.perf > out.folded
	@./FlameGraph/flamegraph.pl out.folded > test.svg
	@cp test.svg /mnt/hgfs/share

clean:
	@sudo rm -f out* perf.data test.svg

