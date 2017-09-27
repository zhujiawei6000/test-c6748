# invoke SourceDir generated makefile for app.pe674
app.pe674: .libraries,app.pe674
.libraries,app.pe674: package/cfg/app_pe674.xdl
	$(MAKE) -f D:\Users\zhuji\Projects\CCD\SY18-201708\DSP\uartTest/src/makefile.libs

clean::
	$(MAKE) -f D:\Users\zhuji\Projects\CCD\SY18-201708\DSP\uartTest/src/makefile.libs clean

