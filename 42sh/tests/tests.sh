#!/bin/sh

REF_OUT=".ref.out"
TEST_OUT=".42sh.out"

testcase_with_c() {
    bash -c "$@" > "$REF_OUT"
    REF_RES=$?
    "../src/42sh" -c "$@" > "$TEST_OUT"
    RES=$?
    if [ "$RES" -eq "$REF_RES" ];then
        echo OK
    else
        echo error "$@"
    fi

}

testcase() {
    bash "$@" > "$REF_OUT"
    REF_RES=$?
    "../src/42sh" "$@" > "$TEST_OUT"
    RES=$?
    if [ "$RES" -eq "$REF_RES" ];then
        echo OK
    else
        echo error "$@"
    fi

}

testcase_as_input() {
    echo "$@" > tested
    bash < tested > "$REF_OUT"
    REF_RES=$?
    "../src/42sh" < tested > "$TEST_OUT"
    RES=$?
    diff $REF_OUT $TEST_OUT
    if [ "$RES" -eq "$REF_RES" ];then
        echo OK
    else
        echo error "$@"
    fi

}

echo ---------------SIMPLE_COMMAND---------------
testcase "script.sh"
testcase_as_input "echo a"
testcase_as_input "ls /bin"
testcase_as_input "cat Makefile.am"
testcase_as_input "wc Makefile"

echo ---------------COMMAND_LIST---------------
testcase_with_c "echo foo; echo bar; echo c"
testcase_as_input "echo a; echo b; echo;"
testcase_as_input "ls /bin; echo a; ls .; cat Makefile; wc -l Makefile"
testcase_as_input "cat Makefile.am; echo a; echo b; echo c"
testcase_as_input "echo echo a; echo echo b; echo echo c;"
testcase_as_input "echo a;                                              echo b;      echo c;echo d"

echo ---------------IF---------------
testcase_as_input "if true; then echo a; else echo b; fi"
testcase_as_input "if false; then echo a; else echo b; fi"
testcase_as_input "if echo a; then echo b; else echo c; fi"
testcase_as_input "if false; then echo a; elif true; then echo b; fi"
testcase_as_input "if false; then false; elif true; then echo a; else echo b; fi"
testcase_as_input "if echo a; then if echo b; then cat Makefile.am; elif true; then echo s; fi; else echo b; fi"
testcase_as_input "if false; then echo a; elif false; then echo b; elif false; then echo c; elif true; then if false; then echo d; elif false; then echo e; else echo f; fi; else echo g; fi"

echo ---------------COMPOUND_LIST---------------
testcase_as_input "if true; false; true; echo a; then echo a; else echo b; fi"
testcase_as_input "if false; then echo a; echo b; echo c; ls; else echo b; echo c; echo d; cat Makefile.am; fi"
testcase_as_input "if echo a
false
then echo b
else echo c; true; echo a; fi"
testcase_as_input "if false;


then echo a;



elif true; true; false; true; then echo b;
echo b; fi"
testcase_as_input "if false; echo a; echo b; true
false
then false; echo c;
ls
echo a; elif true; then echo a;
false;
true;
echo b; else echo b; echo c; echo edfertbb;
fi"
testcase_as_input "if false

then echo a; echo cvrfhnjk,f

elif false; then echo b; elif false; then echo c


elif true; true; true; echo a; then if false; then echo d; elif false


then echo e; else echo f; fi; else echo gfgthy;
echo a; echo dfbng;
fi"

echo ---------------SINGLE_QUOTE---------------
testcase_as_input "echo 'frgdnjkjn' 'fgdhj' gfrdjngdkj\'njfrkeck\'"
# testcase_as_input "echo '(gf)('frngjk '{dqs {fds }fs' '\\\\' fdsbujq\t\e"

echo ---------------TRUE_FALSE---------------
testcase_as_input "true;"
testcase_as_input "false"
testcase_as_input "true; false; true;"
testcase_as_input "true;false;true;false;false"

echo ---------------ECHO---------------
testcase_as_input "echo a"
testcase_as_input "echo a; echo b; echo c; ls /bin; cat Makefile;"
testcase_as_input "if false; then echo bite; elif false; then echo fun; else echo dong; fi"
testcase_as_input "echo 'a b ;d echo if then else ls;;'; echo -n a; echo ';;;;;'"
testcase_as_input "echo \\#escaped \"#\"quoted not#first #commented"
testcase_as_input "echo -e 'abc\n\t\\'; echo -e '\nde\t\n'"
# testcase_as_input "echo -E 'abc'\n\t\\'; echo -E '\nde\t\\\n'"
testcase_as_input "echo -e 'abc\n\t\\'; echo -E '\nde\t\\\n'; echo -E '\n\\\\\\t\d\\\t\n\t\nfe\b'"
testcase_as_input "echo ðß@ß«@ð€đŋ¶ŧŋ←̉↓ħđ€ßøđ€→→€→€→ŋɲŧ↓đ→ŧħ←n↓ŋ→n”←ŋđĸ↓ŋ→ð¶ħ↓€→ßø«æħ¶←đðnħŋ←ŧŋ→€ħßn€đ"

echo ---------------COMMENTS---------------
testcase_with_c "echo \\#escaped \"#\"quoted not#first #commented"
testcase_as_input "echo \\#escaped \"#\"quoted not#first #commented"

echo ---------------REDIRECTIONS---------------
testcase_as_input "ls > temp.txt ; rm temp.txt"
testcase_as_input "cat NOFILE >& temp.txt ; rm temp.txt"
testcase_as_input "ls > temp.txt ; ls < temp.txt ; rm temp.txt"
testcase_as_input "ls >| temp.txt ; rm temp.txt"
testcase_as_input "ls > temp.txt; ls >> temp.txt; ls < temp.txt; rm temp.txt"

echo ---------------PIPELINES---------------
testcase_as_input "echo a | echo b | tr a b"
testcase_as_input "false | true"
testcase_as_input "false | true | true | true"
testcase_as_input "true | false"
testcase_as_input "ls -l | grep test_evaluate.c"
testcase_as_input "echo Hello, World! | tr '[:lower:]' '[:upper:]' | rev | sort | uniq | grep 'L' | sed 's/LO/Hi/' | awk '{print $1}' | cut -c 2-5 | tr '[:lower:]' '[:upper:]'"

echo ---------------NEGATION---------------
testcase_as_input "! true"
testcase_as_input "! false"
testcase_as_input "! true; ! false; ! true"
testcase_as_input "if ! true; then echo a; else echo b; fi"
testcase_as_input "if ! false; ! true; then echo a; else echo b; fi"
testcase_as_input "if ! false; ! true; ! false; then echo a; else echo c; fi"

echo ---------------WHILE---------------
testcase_as_input "while false; do echo a; done"

echo ---------------UNTIL---------------
testcase_as_input "until true; do echo a; done"

echo ---------------AND_OR---------------
testcase_as_input "true && false"
testcase_as_input "false && true; true || false"
testcase_as_input "false && true && false && true || false || false || true && false"
testcase_as_input "echo a && echo b || echo c || echo d"

echo ---------------DOUBLE_QUOTES/ESCAPED---------------
testcase_as_input "echo \"fbhjf\""
# testcase_as_input "echo \"\\\\\""
# testcase_as_input "echo \"\" \"dsefnhj\\\t\\t\\n\""
# testcase_as_input "echo \"\t\n\t\\t\\n\\edsev\fesnkqz\""
# testcase_as_input "echo \\t\\n\\t\\t\t\n\t\"\"\'\$\("
# testcase_as_input "echo \t\\t\b\n\\}t\e\"\"\'\'\$\(\)\(\{\(\)"
# testcase_as_input "echo \t\\t\b\n\\}t\e\"\"\'\'\$\(\)\(\{\(\)"

echo ---------------ASSIGNMENT_WORD---------------
testcase_as_input "a=2; echo $a"
testcase_as_input "a=42; b=5; echo $a $b; a=948152; b=4; b=486; echo -n $a; echo $b"
testcase_with_c "echo $a; a=1; echo $a && echo $b; b=156; echo $a a $a$b"
testcase_as_input "abcd=89; echo ${abcd} && echo $abcd"
# testcase_as_input "echo '\$titi'"
testcase_as_input "titi=jaimelacarotte; echo \"\$titi\" \"\$titi\"; echo \"\$titi\""
testcase_as_input "if true; then echo \$?; fi; echo \$?"
testcase_as_input "test() { echo tg;} ; echo $?"
testcase_as_input "test() { f;} ; echo $?"
testcase_as_input "while true; do echo $? && break; done; echo $?"

echo ---------------FOR---------------
testcase_as_input "for i in 1 2 3 4 5 6; do echo a; done"
testcase_with_c "for i in 1 2 3 4 5 6; do echo $i; done"
testcase_as_input "for i in echo a; do echo b; done"
testcase_as_input "for i; do echo a; done"
testcase_as_input "for i in `echo a`; do echo $i; done"

echo ---------------COMMAND_BLOCKS---------------
testcase_as_input "{ echo a; echo b; } | tr b h"

echo ---------------SUBSHELLS---------------
testcase_as_input "a=sh; (a=42; echo -n $a);echo $a"
testcase_as_input "a=sh; (ls);echo $a; (a=69;(a=12; echo $a); echo $a); (a=12; echo $a); echo $a"
testcase_as_input "a=sh; b=feur; (ls);echo $a; (a=69;(a=12;(b=quoi;echo $b);echo $b; echo $a); echo $a); echo $a"

echo ---------------CONTINUE/BREAK---------------
testcase_as_input "for i in 1 2 3 4 5 ; do if [ \$i -eq 3 ]; then continue 3; else echo \$i \$i;fi ;done "
testcase_as_input "for i in 1 2 3 4 5 ; do if [ \$i -eq 3 ]; then break ; else echo \$i \$i;fi ;done "
testcase_as_input "for i in 1 2 3 4 5 ; do if [ \$i -eq 3 ]; then break; echo bite ; else echo \$i \$i;fi ;done "
testcase_as_input "for i in 1 2 3 4 5 ; do if [ \$i -eq 3 ]; then continue; echo  bite ; else echo \$i \$i;fi ;done "

echo ---------------UNSET---------------
testcase_as_input "a=2; b=5; echo \$a; unset a; echo \$b; unset -v b; echo \$a \$b"
# testcase_as_input "a=2; b=txt; echo \$a; unset a; echo \$b; unset -f b; echo \$a \$b"
testcase_as_input "a=2; b=5; echo $a; unset a; echo $b; unset -v b; echo $a $b; b=69; echo $b"
testcase_as_input "a=2; a=5; echo $a; unset a; echo $a; unset -v a; echo $a; a=69; echo $a"
testcase_as_input "a=2; a=5; echo $a; unset a; echo $a; unset -v a; unset -v a;unset -v a; unset -v a; echo $a; a=69; echo $a"
testcase_as_input "test() {
                    echo bite
                    }
                    test;
                   unset -f test"
echo ---------------CD---------------
testcase_as_input "cd /; pwd; cd ..; pwd"
testcase_as_input "echo $OLDPWD $PWD; ls; cd ..; echo $OLDPWD $PWD; ls"

echo ---------------DOT---------------
testcase_as_input ". ../tests/dot.sh"

echo -------------FUNCTION------------
testcase_as_input  "afficher_fichiers() {
    echo Les fichiers dans le répertoire courant sont :; ls
    }
    afficher_fichiers"
testcase_as_input "test() { test2() { echo t ; ls; } } ; test ; test2"
testcase_as_input "test() {
    test2() {
        test3() {
            echo je pense que ca marche
            }
            }
    }
    test
    test2
    test3"
testcase_as_input "test() {
    echo ancien en fait
    } ; test() {
        echo nouveau en fait
        }
    test"

testcase_as_input "test() { echo \$1; }; test echo"
