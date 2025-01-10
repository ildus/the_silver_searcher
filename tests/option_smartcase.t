Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'asdf\n' > test.txt
  $ printf 'AsDf\n' >> test.txt

Smart case search:

  $ ag -S -G "test.txt" asdf
  test.txt:1:asdf
  test.txt:2:AsDf

Order of options should not matter:

  $ ag -G "test.txt" -S asdf
  test.txt:1:asdf
  test.txt:2:AsDf
