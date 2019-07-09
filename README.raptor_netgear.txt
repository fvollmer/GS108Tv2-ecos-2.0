/* mips_raptor_netgear.ecc is customized to netgear platforms(BCM5331x series) like gs110t and gs510tp */

COMPILATION procedure:

[ecos-2.0--src]$ chmod 0777 mips_raptor_netgear.ecc

1)Export the Tool chain path as below.
 
[ecos-2.0--src]$ export PATH=$PATH:/tools/ecos/mipsisa32-elf/bin/
 
2)Remove the previously generated header files & lib files , if any

[ecos-2.0--src]$ rm -r output/ecos-2.0--raptor_netgear/

3)Compile with appropriate CPU option

[ecos-2.0--src]$ make L7_CPU=raptor_netgear


4)Copy the ecos libs (i.e include/ libs/ directories) from "output/ecos-2.0--raptor_netgear/install" to 

"bsp/cpu/raptor/ecos/ecos-libs/" instead of "bsp/platform/broadcom/<platform>/ecos-libs/"




