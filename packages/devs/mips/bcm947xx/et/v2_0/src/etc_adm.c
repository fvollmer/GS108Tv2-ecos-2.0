/*
 * ADMtek switch setup functions
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id: etc_adm.c,v 1.1 Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/sbutils.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/bcmendian.h>

#include <et_dbg.h>

/* Private state per ADM switch */
typedef struct {
	void *sbh;			/* SiliconBackplane handle */
	uint coreidx;			/* Current core index */
	uint8 eecs, eesk, eedi;		/* GPIO mapping */
} adm_info_t;

/* Minimum timing constants */
#define EECK_EDGE_TIME	3	/* 3us - max(adm 2.5us, 93c 1us) */
#define EEDI_SETUP_TIME	1	/* 1us - max(adm 10ns, 93c 400ns) */
#define EECS_SETUP_TIME	1	/* 1us - max(adm no, 93c 200ns) */

/* Forward declarations */
adm_info_t *adm_attach(void *sbh, char *vars);
void adm_detach(adm_info_t *adm);
void adm_enable_device(adm_info_t *adm, char *vars);
int adm_config_vlan(adm_info_t *adm, char *vars);

/* Return gpio number assigned to the named pin */
/*
* Variable should be in format:
*
*	gpio<N>=pin_name
*
* 'def_gpio' is returned if there is no such variable is found.
*/
#define NUMGPIO	16
static uint8
adm_get_gpio(char *vars, char *pin_name, uint8 def_gpio)
{
	char name[] = "gpioXXXXXXXX";
	char *val;
	uint8 i;

	/* Go thru all possibilities till a match in pin name */
	for (i = 0; i < NUMGPIO; i ++) {
		sprintf(name, "gpio%d", i);
		val = getvar(vars, name);
		if (val && !strcmp(val, pin_name))
			return (1 << i);
	}
	return def_gpio;
}

/* Allocate private resource */
adm_info_t *
adm_attach(void *sbh, char *vars)
{
	adm_info_t *adm;

	/* Allocate private data */
	if (!(adm = MALLOC(sizeof(adm_info_t)))) {
		ET_ERROR(("adm_attach: out of memory"));
		return NULL;
	}
	bzero((char *) adm, sizeof(adm_info_t));
	adm->sbh = sbh;

	/* Init GPIO mapping. Default GPIO: 2, 3, 4 */
	adm->eecs = (1 << 2);
	adm->eesk = (1 << 3);
	adm->eedi = (1 << 4);

	/* nvram overrides */
	adm->eecs = adm_get_gpio(vars, "adm_eecs", adm->eecs);
	adm->eesk = adm_get_gpio(vars, "adm_eesk", adm->eesk);
	adm->eedi = adm_get_gpio(vars, "adm_eedi", adm->eedi);


	return adm;
}

/* Release private resource */
void
adm_detach(adm_info_t *adm)
{
	/* Free private data */
	MFREE(adm, sizeof(adm_info_t));
}

/*
* The following local functions provide chip access control. The 
* general rules in writing these supporting routines are:
*
*   1. EECS should be kept low after each routine.
*   2. EESK should be kept low after each routine.
*/


/* Enable outputs with specified value to the chip */
static void
adm_enout(adm_info_t *adm, uint8 pins, uint8 val)
{
	/* Prepare GPIO output value */
	sb_gpioout(adm->sbh, pins, val);
	/* Enable GPIO outputs */
	sb_gpioouten(adm->sbh, pins, pins);
	OSL_DELAY(EECK_EDGE_TIME);
}

/* Disable outputs to the chip */
static void
adm_disout(adm_info_t *adm, uint8 pins)
{
	/* Disable GPIO outputs */
	sb_gpioouten(adm->sbh, pins, 0);
	OSL_DELAY(EECK_EDGE_TIME);
}

/* Advance clock(s) */
static void
adm_adclk(adm_info_t *adm, int clocks)
{
	int i;
	for (i = 0; i < clocks; i ++) {
		/* Clock high */
		sb_gpioout(adm->sbh, adm->eesk, adm->eesk);
		OSL_DELAY(EECK_EDGE_TIME);
		/* Clock low */
		sb_gpioout(adm->sbh, adm->eesk, 0);
		OSL_DELAY(EECK_EDGE_TIME);
	}
}	

/* Write a bit stream to the chip */
static void
adm_write(adm_info_t *adm, uint8 cs, uint8 *buf, uint bits)
{
	uint i, len = (bits + 7) / 8;
	uint8 mask;

	/* CS high/low */
	if (cs)
		sb_gpioout(adm->sbh, adm->eecs, adm->eecs);
	else
		sb_gpioout(adm->sbh, adm->eecs, 0);
	OSL_DELAY(EECK_EDGE_TIME);

	/* Byte assemble from MSB to LSB */
	for (i = 0; i < len; i++) {
		/* Bit bang from MSB to LSB */
		for (mask = 0x80; mask && bits > 0; mask >>= 1, bits --) {
			/* Clock low */
			sb_gpioout(adm->sbh, adm->eesk, 0);
			OSL_DELAY(EECK_EDGE_TIME);

			/* Output on rising edge */
			if (mask & buf[i])
				sb_gpioout(adm->sbh, adm->eedi, adm->eedi);
			else
				sb_gpioout(adm->sbh, adm->eedi, 0);
			OSL_DELAY(EEDI_SETUP_TIME);
		
			/* Clock high */
			sb_gpioout(adm->sbh, adm->eesk, adm->eesk);
			OSL_DELAY(EECK_EDGE_TIME);
		}
	}

	/* Clock low */
	sb_gpioout(adm->sbh, adm->eesk, 0);
	OSL_DELAY(EECK_EDGE_TIME);

	/* CS low */
	if (cs)
		sb_gpioout(adm->sbh, adm->eecs, 0);
}

/* Handy macros for writing fixed length values */
#define adm_write8(adm, cs, b) { uint8 val = (uint8) (b); adm_write(adm, cs, &val, sizeof(val)*8); }
#define adm_write16(adm, cs, w) { uint16 val = hton16(w); adm_write(adm, cs, (uint8 *)&val, sizeof(val)*8); }
#define adm_write32(adm, cs, i) { uint32 val = hton32(i); adm_write(adm, cs, (uint8 *)&val, sizeof(val)*8); }

/* Write chip configuration register */
/* Follow 93c66 timing and chip's min EEPROM timing requirement */
static void
adm_wreg(adm_info_t *adm, uint8 addr, uint16 val)
{
	/* cmd(27bits): sb(1) + opc(01) + addr(bbbbbbbb) + data(bbbbbbbbbbbbbbbb) */
	uint8 bits[4] = {
		(0x05 << 5) | (addr >> 3),
		(addr << 5) | (uint8)(val >> 11),
		(uint8)(val >> 3),
		(uint8)(val << 5)
	};

	ET_TRACE(("adm_wreg: addr %02x val %04x (%02X%02X%02X%02X)\n", 
		addr, val, bits[0], bits[1], bits[2], bits[3]));

	/* Enable GPIO outputs with all pins to 0 */
	adm_enout(adm, (uint8)(adm->eecs | adm->eesk | adm->eedi), 0);

	/* Write cmd. Total 27 bits */
	adm_write(adm, 1, bits, 27);

	/* Extra clock(s) required per datasheet */
	adm_adclk(adm, 2);

	/* Disable GPIO outputs */
	adm_disout(adm, (uint8)(adm->eecs | adm->eesk | adm->eedi));
}


/* Copy each token in wordlist delimited by space into word */
static int _strspn(const char * p, const char * s)
{
	int i,j;

	for (i=0;p[i];i++) {
		for (j=0;s[j];j++) {
			if (s[j] == p[i]) break;
		}
		if (!s[j]) break;
	}
	return(i);
}
static int _strcspn(char *p, char *s)
{
	int i,j;

	for (i=0;p[i];i++) {
		for (j=0;s[j];j++) {
			if (s[j] == p[i]) break;
		}
		if (s[j]) break;
	}
	return(i);
}
#define foreach(word, wordlist, next) \
	for (next = &wordlist[_strspn(wordlist, " ")], \
	     strncpy(word, next, sizeof(word)), \
	     word[_strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '); \
	     strlen(word); \
	     next = next ? &next[_strspn(next, " ")] : "", \
	     strncpy(word, next, sizeof(word)), \
	     word[_strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '))

/* Maximum vlan groups */
#define MAX_VLAN_GROUPS	16

/* Port configuration */
typedef struct {
	uint8 addr;	/* port configuration register */
	uint16 vlan;	/* vlan port mapping */
	uint8 tagged;	/* output tagging */
	uint8 cpu;	/* cpu port? 1 - yes, 0 - no */
	uint16 pvid;	/* cpu port pvid */
} port_cfg;

/* Vlan port map */
typedef struct {
	uint8 addr;	/* vlan port map register */
} vlan_cfg;

/* Configure the chip based on nvram settings */
int
adm_config_vlan(adm_info_t *adm, char *vars)
{
	port_cfg port_cfg_tab[] = {
		{1, 1<<0, 0, 0, -1},
		{3, 1<<2, 0, 0, -1},
		{5, 1<<4, 0, 0, -1},
		{7, 1<<6, 0, 0, -1},
		{8, 1<<7, 0, 0, -1},
#if defined(PMON) || defined(_CFE_)
		{9, 1<<8, 0, 1, -1}	/* no output tagging for pmon/cfe */
#else	/* #if defined(PMON) || defined(CFE) */
		{9, 1<<8, 1, 1, -1}	/* output tagging for linux... */
#endif	/* #if defined(PMON) || defined(CFE) */
	};
	vlan_cfg vlan_cfg_tab[] = {
		{0x13},
		{0x14},
		{0x15},
		{0x16},
		{0x17},
		{0x18},
		{0x19},
		{0x1a},
		{0x1b},
		{0x1c},
		{0x1d},
		{0x1e},
		{0x1f},
		{0x20},
		{0x21},
		{0x22}
	};
	uint16 i;

	/* vlan mode select register (0x11): vlan on, mac clone */
	adm_wreg(adm, 0x11, 0xff30);

	/* vlan port group: port configuration, vlan port map */
	for (i = 0; i < MAX_VLAN_GROUPS; i ++) {
		char port[16], *next;
		char vlan_group[16];
		uint16 vlan_map = 0;
		char *ports;

		/* get nvram port settings */
		sprintf(vlan_group, "vlan%dports", i);
		ports = getvar(vars, vlan_group);
		if (!ports)
			continue;

		/*
		* port configuration register (0x01, 0x03, 0x05, 0x07, 0x08, 0x09): 
		*   input/output tagging, pvid, auto mdix, auto negotiation, ...
		* cpu port needs special handing to support pmon/cfe/linux...
		*/
		foreach (port, ports, next) {
			int port_num = bcm_atoi(port);
			uint16 port_cfg;
			
			/* make sure port # is within the range */
			if (port_num >= sizeof(port_cfg_tab) / sizeof(port_cfg_tab[0])) {
				ET_ERROR(("port number %d is out of range\n", port_num));
				continue;
			}
			
			/* build vlan port map */
			vlan_map |= port_cfg_tab[port_num].vlan;
			
			/* cpu port needs special care */
			if (port_cfg_tab[port_num].cpu) {
				/* check if the port is marked */
				if (strchr(port, '*'))
					port_cfg_tab[port_num].pvid = i;
				/* will be done later */
				continue;
			}
			
			/* configure port */
			port_cfg = 0x8000 |	/* auto mdix */
				(i << 10) | 	/* pvid */
				0x000f;		/* full duplex, 100Mbps, auto neg, flow ctrl */
			adm_wreg(adm, port_cfg_tab[port_num].addr, port_cfg);
		}

		/* vlan port map register (0x13 - 0x22) */
		adm_wreg(adm, vlan_cfg_tab[i].addr, vlan_map);
	}
	
	/* cpu port config: auto mdix, pvid, output tagging, ... */
	for (i = 0; i < sizeof(port_cfg_tab)/sizeof(port_cfg_tab[0]); i ++) {
		uint16 tagged, pvid;
		uint16 port_cfg;
		
		/* cpu port only */
		if (port_cfg_tab[i].cpu == 0 || port_cfg_tab[i].pvid == 0xffff)
			continue;
		
		/* configure port */
		tagged = port_cfg_tab[i].tagged ? 1 : 0;
		pvid = port_cfg_tab[i].pvid;
		port_cfg = 0x8000 |	/* auto mdix */
			(pvid << 10) | 	/* pvid */
			(tagged << 4) |	/* output tagging */
			0x000f;		/* full duplex, 100Mbps, auto neg, flow ctrl */
		adm_wreg(adm, port_cfg_tab[i].addr, port_cfg);
	}
	
	return 0;
}

/*
* Enable the chip with preset default configuration:
*
*  TP Auto MDIX (EESK/GPIO = 1)
*  Single Color LED (EEDI/GPIO = 0)
*  EEPROM Disable (H/W pull down)
*/
void
adm_enable_device(adm_info_t *adm, char *vars)
{
	uint8 rc;
	int i;
	
	/* Check nvram override existance */
	if ((rc = adm_get_gpio(vars, "adm_rc", 0xff)) == 0xff)
		return;
	
	/*
	* Reset sequence: RC high->low(100ms)->high(30ms)
	*
	* WAR: Certain boards don't have the correct power on 
	* reset logic therefore we must explicitly perform the
	* sequece in software.
	*/
	/* Keep RC high for at least 20ms */
	adm_enout(adm, rc, rc);
	for (i = 0; i < 20; i ++)
		OSL_DELAY(1000);
	/* Keep RC low for at least 100ms */
	adm_enout(adm, rc, 0);
	for (i = 0; i < 100; i++)
		OSL_DELAY(1000);
	/* Set default configuration */
	adm_enout(adm, (uint8)(adm->eesk | adm->eedi), adm->eesk);
	/* Keep RC high for at least 30ms */
	adm_enout(adm, rc, rc);
	for (i = 0; i < 30; i++)
		OSL_DELAY(1000);
	/* Leave RC high and disable GPIO outputs */
	adm_disout(adm, (uint8)(adm->eecs | adm->eesk | adm->eedi));
}

