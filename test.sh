#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" >tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 1 '-(3+5)+9;'
assert 8 '-3*+5+23;'
assert 1 '23==23;'
assert 0 '21==2;'
assert 2 '1+(5<=123);'
assert 1 '24<=123;'
assert 1 '3<4;'
assert 0 '2>=123*3;'
assert 1 '7>1;'
assert 0 '1!=1;'
assert 5 'a=5;'
assert 1 'q=5<10;'
assert 16 'z=(2*(5+3));'
assert 24 '6;
y=3*8;'
assert 84 'abc=6; e=2*4; zzz=abc+e; zzz=zzz*abc;'
assert 0 'return 0;'
assert 52 'return 52; return 12;'
assert 15 'a=3; b=5; return a*b;'

echo OK
