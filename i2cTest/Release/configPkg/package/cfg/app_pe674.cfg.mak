# invoke SourceDir generated makefile for app.pe674
app.pe674: .libraries,app.pe674
.libraries,app.pe674: package/cfg/app_pe674.xdl
	$(MAKE) -f D:\Users\zhuji\Documents\workspace_v7\i2cTest/src/makefile.libs

clean::
	$(MAKE) -f D:\Users\zhuji\Documents\workspace_v7\i2cTest/src/makefile.libs clean

