bin_PROGRAMS = \
	gloox-ft-test

gloox_ft_test_CPPFLAGS = \
	`pkg-config --cflags gloox`
	
gloox_ft_test_LDFLAGS = \
	-pthread \
	`pkg-config --libs gloox`
	
gloox_ft_test_SOURCES = \
	common.h \
	main.cxx \
	receiv.cxx \
	sender.cxx
