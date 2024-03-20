check_PROGRAMS = criterion

criterion_SOURCES = \
	test_parser.c \
	test_lexer.c \
	test_evaluate.c

criterion_CPPFLAGS = -I$(top_srcdir)/src

criterion_LDADD = \
	-lcriterion \
	$(top_builddir)/src/parser/libparser.a \
	$(top_builddir)/src/lexer/liblexer.a \
	$(top_builddir)/src/ast/libast.a \
	$(top_builddir)/src/evaluate/libevaluate.a

check-local: criterion
	./criterion
	./tests.sh
