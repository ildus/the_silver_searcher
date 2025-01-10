Setup and create really big file:

  $ . $TESTDIR/../setup.sh
  $ python3 $TESTDIR/create_big_file.py $TESTDIR/big_file.txt
  $ cp $TESTDIR/big_file.txt /tmp/big_file.txt

Search a big file:

  $ $TESTDIR/../../ag --nocolor --workers=1 --parallel hello $TESTDIR/big_file.txt
  33554432:hello1073741824
