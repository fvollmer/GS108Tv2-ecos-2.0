COMPILATION procedure:

[ecos-2.0--src]$chmod 0777 mips_keystone.ecc 

1)Export the Tool chain path as below.
 
[ecos-2.0--src]$ export PATH=$PATH:/tools/ecos/mipsisa32-elf/bin/
 
2)Remove the previously generated header files & lib files , if any

[ecos-2.0--src]$ rm -r output/ecos-2.0--keystone/

3)Compile with appropriate CPU option

[ecos-2.0--src]$ make L7_CPU=keystone

4)Copy the ecos libs (i.e include/ libs/ directories) from "output/ecos-2.0--keystone/install" to
"bsp/cpu/keystone/ecos/ecos-libs/" instead of "bsp/platform/broadcom/<platform>/ecos-libs/"

NOTE: 
 
Ensure that the following part of the code should be commented in generated header file at “include\cyg\hal\bcmnvram.h” in order to avoid compilation errors.
 
#if 0
/*
 * Initialize NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern int nvram_init(void *sih);
 
/*
 * Append a chunk of nvram variables to the global list
 */
extern int nvram_append(void *si, char *vars, uint varsz);
 
/*
 * Check for reset button press for restoring factory defaults.
 */
extern bool nvram_reset(void *sih);
 
/*
 * Disable NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern void nvram_exit(void *sih);
#endif
