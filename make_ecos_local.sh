
MYBIN=/tmp/mybin.$USER
export PATH=$MYBIN:$PATH
mkdir -p $MYBIN
echo "/usr/bin/tail \$2 | grep -v ':'" > $MYBIN/tail
chmod +x $MYBIN/tail

cd new_install
make -C .
