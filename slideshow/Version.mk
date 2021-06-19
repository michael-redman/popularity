#Version.mk - Copyright 2021 Michael Redman - https://www.michael-redman.name
#MIT/BSD License

#"include Version.mk" (without quotes) in your Makefile to:
#	- store the current "git describe" output in VERSION Makefile variable
#	- automatically "make clean" whenever this changes

#Do not remove the "VERSION" file in your "make clean".

#On CentOS you have to change the "!=" syntax to ":=$(shell ....)" syntax, example:
#VERSION:=$(shell git describe --always --tags --dirty)

VERSION!=git describe --always --tags --dirty
VERSION_CHECK!=echo $(VERSION) > /tmp/$$$$ && cmp -s /tmp/$$$$ VERSION || ( cat /tmp/$$$$ > VERSION && make clean ) && rm /tmp/$$$$ && echo $(VERSION)

#IN GOD WE TRVST.
