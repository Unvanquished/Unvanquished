TestCPUInfo: SimpleTestCPUInfo.cpp libcpuinfo.a
	c++ -o TestCPUInfo SimpleTestCPUInfo.cpp -L. -lcpuinfo
	
libcpuinfo.a: CPUInfo.o
	ar rv libcpuinfo.a CPUInfo.o
	ranlib libcpuinfo.a
	
CPUInfo.o: CPUInfo.c
	cc -c -o CPUInfo.o CPUInfo.c

clean:
	rm -rf *.o libcpuinfo.a TestCPUInfo

depend:
	cc -E -MM *.c* > .depend
