TEMPLATE = subdirs
SUBDIRS  = \
	test_filters \
	test_sessions

test.target = test
test.CONFIG = recursive
QMAKE_EXTRA_TARGETS += test
