/* mips_raptor_netgear.ecc is customized to netgear platforms(BCM5331x series) like gs110t and gs510tp */

COMPILATION procedure:


1) Get toolchain: 

wget http://mirrors.kernel.org/sources.redhat.com/ecos/gnutools/i386linux/ecoscentric-gnutools-mipsisa32-elf-1.4-2.i386linux.tar.bz2

2) Extract toolchain:

tar -xvjf ecoscentric-gnutools-mipsisa32-elf-1.4-2.i386linux.tar.bz2

3) Add to toolchain to path
 
export PATH=$PATH:/path/to/gnutools/mipsisa32-elf/bin/
 
4) Compile with appropriate CPU option

[GS108Tv2-ecos-2.0]$ make L7_CPU=raptor_netgear

5) Build your application. For example:

[GS108Tv2-ecos-2.0]$ cd examples/
[GS108Tv2-ecos-2.0/examples]$ make INSTALL_DIR=/path/to/GS108Tv2-ecos-2.0/output/ecos-2.0--raptor_netgear/install

This created a file "hello", that you can now boot
