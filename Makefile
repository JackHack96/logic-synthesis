bin: all
	-mkdir bin
	mv espresso/espresso ./bin
all: 
	-mkdir lib include doc
	(cd utility; make ../include/utility.h CAD=..)
	for i in port uprintf errtrap st mm utility; do \
		(cd $$i; make install CAD=..); done
	(cd espresso; make CAD=..)

clean:
	rm -rf ./bin
	for i in port uprintf errtrap st mm utility espresso; do \
		(cd $$i; make clean CAD=..); done 
