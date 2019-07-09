#export  ECOS_REPOSITORY=$PWD/packages

#MYBIN=/tmp/mybin.$USER
#export PATH=$MYBIN:$PATH
#mkdir -p $MYBIN
#echo "/usr/bin/tail \$2 | grep -v ':'" > $MYBIN/tail
#chmod +x $MYBIN/tail

INSTALL=./output/ecos-2.0--$2/install
mipsisa32-elf-gcc -g -o $1 -I. -I$INSTALL/include/ ./examples/$1.c -L $INSTALL/lib/ -T$INSTALL/lib/target.ld -nostdlib -EB -Wl,-Map,target.map
