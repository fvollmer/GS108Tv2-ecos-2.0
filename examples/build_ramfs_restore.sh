#export ECOS_REPOSITORY=/home2/sastry/ecos/ecos-2.0
#export PATH=/home2/sastry/ecos/ecos-2.0/tools/bin:$PATH
#export PATH=~/temp-ecos/ecos_new/gnutools/mipsisa32-elf/bin:$PATH

MYBIN=/tmp/mybin.$USER
export PATH=$MYBIN:$PATH
mkdir -p $MYBIN
echo "/usr/bin/tail \$2 | grep -v ':'" > $MYBIN/tail
chmod +x $MYBIN/tail

#cd wss-turnkey-1.1.2-rc2/systems/ecos/raptor
#make
#cd -

#rm -rf $MYBIN

rm -f switchdrvr

INSTALL=./new_install/install
mipsisa32-elf-gcc -g -o switchdrvr -I. -I$INSTALL/include/ ramfs_restore.c -L $INSTALL/lib/ -Ttarget.ld -nostdlib -EB -Wl,-Map,target.map
