Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'Foo\n' >> ./sample
  $ printf 'bar\n' >> ./sample

Smart case by default:

  $ ag foo sample
  1:Foo
  $ ag FOO sample
  [1]
  $ ag 'f.o' sample
  1:Foo
  $ ag Foo sample
  1:Foo
  $ ag 'F.o' sample
  1:Foo

Case sensitive mode:

  $ ag -s foo sample
  [1]
  $ ag -s FOO sample
  [1]
  $ ag -s 'f.o' sample
  [1]
  $ ag -s Foo sample
  1:Foo
  $ ag -s 'F.o' sample
  1:Foo

Case insensitive mode:

  $ ag -i fOO sample
  1:Foo
  $ ag --ignore-case fOO sample
  1:Foo
  $ ag -i 'f.o' sample
  1:Foo

Case insensitive file regex

  $ ag -i  -g 'Samp.*'
  sample
