# #!bin/sh

make clean
make distclean

rm -rf missing
rm -rf Makefile.in
rm -rf aclocal.m4
rm -rf autom4te.cache
rm -rf compile
rm -rf configure
rm -rf depcomp
rm -rf install-sh
rm -rf config.log
rm -rf config.status
rm -rf Makefile
rm -rf ar-lib
rm ast.dot ast.png

rm -rf src/.d* src/Makefile.in src/ast.dot
rm -rf src/ast/.d* src/ast/Makefile.in
rm -rf src/parser/.d* src/parser/Makefile.in
rm -rf src/lexer/.d* src/lexer/Makefile.in
rm -rf src/evaluate/.d* src/evaluate/Makefile.in
rm -rf tests/.d* tests/Makefile.in tests/.42sh.out tests/.ref.out tests/ast.dot tests/tested
