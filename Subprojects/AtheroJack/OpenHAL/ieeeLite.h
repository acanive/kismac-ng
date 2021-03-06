/*
 *  ieeeLite.h
 *  AtheroJack
 *
 *  Created by mick on 4/29/05.
 *  Copyright 2006 Michael Rossberg, Beat Zahnd. All rights reserved.
 *
 */

#include <sys/types.h>

#ifndef IEEELITE
#define IEEELITE

#pragma mark -
#pragma mark OpenBSD src/sys/net80211/ieee80211.h

/*
 * Definitions ported from OpenBSD src/sys/net80211/ieee80211.h r1.10
 */

#define	IEEE80211_ADDR_LEN	6		/* size of 802.11 address */
/* is 802.11 address multicast/broadcast? */
#define	IEEE80211_IS_MULTICAST(_a)	(*(_a) & 0x01)


/* 
 * Channel attributes
 */
#define	IEEE80211_CHAN_TURBO	0x0010	/* Turbo channel */
#define	IEEE80211_CHAN_CCK	0x0020	/* CCK channel */
#define	IEEE80211_CHAN_OFDM	0x0040	/* OFDM channel */
#define	IEEE80211_CHAN_2GHZ	0x0080	/* 2 GHz spectrum channel. */
#define	IEEE80211_CHAN_5GHZ	0x0100	/* 5 GHz spectrum channel */
#define	IEEE80211_CHAN_PASSIVE	0x0200	/* Only passive scan allowed */
#define	IEEE80211_CHAN_DYN	0x0400	/* Dynamic CCK-OFDM channel */
#define	IEEE80211_CHAN_GFSK	0x0800	/* GFSK channel (FHSS PHY) */
#define	IEEE80211_CHAN_XR	0x1000	/* Extended range OFDM channel */

/*
 * Useful combinations of channel characteristics.
 */
#define	IEEE80211_CHAN_FHSS \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_GFSK)
#define	IEEE80211_CHAN_A \
	(IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM)
#define	IEEE80211_CHAN_B \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_CCK)
#define	IEEE80211_CHAN_PUREG \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM)
#define	IEEE80211_CHAN_G \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN)
#define	IEEE80211_CHAN_T \
	(IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)
#define	IEEE80211_CHAN_TG \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)


#define	IEEE80211_WEP_KEYLEN			5	/* 40bit */
#define	IEEE80211_WEP_NKID			4	/* number of key ids */

/* WEP header constants */
#define	IEEE80211_WEP_IVLEN			3	/* 24bit */
#define	IEEE80211_WEP_KIDLEN			1	/* 1 octet */
#define	IEEE80211_WEP_CRCLEN			4	/* CRC-32 */
#define	IEEE80211_CRC_LEN			4
#define	IEEE80211_WEP_TOTLEN		(IEEE80211_WEP_IVLEN + \
					 IEEE80211_WEP_KIDLEN + \
					 IEEE80211_WEP_CRCLEN)

/*
 * 802.11i defines an extended IV for use with non-WEP ciphers.
 * When the EXTIV bit is set in the key id byte an additional
 * 4 bytes immediately follow the IV for TKIP.  For CCMP the
 * EXTIV bit is likewise set but the 8 bytes represent the
 * CCMP header rather than IV+extended-IV.
 */
#define	IEEE80211_WEP_EXTIV		0x20
#define	IEEE80211_WEP_EXTIVLEN		4	/* extended IV length */
#define	IEEE80211_WEP_MICLEN		8	/* trailing MIC */

/*
 * Maximum acceptable MTU is:
 *	IEEE80211_MAX_LEN - WEP overhead - CRC -
 *		QoS overhead - RSN/WPA overhead
 * Min is arbitrarily chosen > IEEE80211_MIN_LEN.  The default
 * mtu is Ethernet-compatible; it's set by ether_ifattach.
 */
#define	IEEE80211_MTU_MAX			2290
#define	IEEE80211_MTU_MIN			32

#define	IEEE80211_MAX_LEN			(2300 + IEEE80211_CRC_LEN + \
    (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN))

#pragma mark -
#pragma mark OpenBSD src/sys/net80211/ieee80211_var.h

/*
 * Definitions ported from OpenBSD src/sys/net80211/ieee80211_var.h r1.12
 */

#define	IEEE80211_CHAN_MAX	255
#define	IEEE80211_CHAN_ANY	0xffff		/* token for ``any channel'' */
#define	IEEE80211_CHAN_ANYC \
	((struct ieee80211_channel *) IEEE80211_CHAN_ANY)

#define	IEEE80211_TXPOWER_MAX	100	/* max power */
#define	IEEE80211_TXPOWER_MIN	-50	/* kill radio (if possible) */

enum ieee80211_phytype {
	IEEE80211_T_DS,			/* direct sequence spread spectrum */
	IEEE80211_T_FH,			/* frequency hopping */
	IEEE80211_T_OFDM,		/* frequency division multiplexing */
	IEEE80211_T_TURBO,		/* high rate OFDM, aka turbo mode */
	IEEE80211_T_XR		        /* extended range mode */
};
#define	IEEE80211_T_CCK	IEEE80211_T_DS	/* more common nomenclature */

/* XXX not really a mode; there are really multiple PHY's */
enum ieee80211_phymode {
	IEEE80211_MODE_AUTO	= 0,	/* autoselect */
	IEEE80211_MODE_11A	= 1,	/* 5GHz, OFDM */
	IEEE80211_MODE_11B	= 2,	/* 2GHz, CCK */
	IEEE80211_MODE_11G	= 3,	/* 2GHz, OFDM */
	IEEE80211_MODE_FH	= 4,	/* 2GHz, GFSK */
	IEEE80211_MODE_TURBO	= 5	/* 5GHz, OFDM, 2x clock */
};
#define	IEEE80211_MODE_MAX	(IEEE80211_MODE_TURBO+1)

enum ieee80211_opmode {
	IEEE80211_M_STA		= 1,	/* infrastructure station */
	IEEE80211_M_IBSS	= 0,	/* IBSS (adhoc) station */
	IEEE80211_M_AHDEMO	= 3,	/* Old lucent compatible adhoc demo */
	IEEE80211_M_HOSTAP	= 6,	/* Software Access Point */
	IEEE80211_M_MONITOR	= 8	/* Monitor mode */
};

/*
 * 802.11g protection mode.
 */
enum ieee80211_protmode {
	IEEE80211_PROT_NONE	= 0,	/* no protection */
	IEEE80211_PROT_CTSONLY	= 1,	/* CTS to self */
	IEEE80211_PROT_RTSCTS	= 2	/* RTS-CTS */
};

/*
 * Channels are specified by frequency and attributes.
 */
struct ieee80211_channel {
	u_int16_t	ic_freq;	/* setting in Mhz */
	u_int16_t	ic_flags;	/* see below */
};

#define	IEEE80211_IS_CHAN_FHSS(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_FHSS) == IEEE80211_CHAN_FHSS)
#define	IEEE80211_IS_CHAN_A(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_A) == IEEE80211_CHAN_A)
#define	IEEE80211_IS_CHAN_B(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_B) == IEEE80211_CHAN_B)
#define	IEEE80211_IS_CHAN_PUREG(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_PUREG) == IEEE80211_CHAN_PUREG)
#define	IEEE80211_IS_CHAN_G(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_G) == IEEE80211_CHAN_G)
#define	IEEE80211_IS_CHAN_T(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_T) == IEEE80211_CHAN_T)
#define	IEEE80211_IS_CHAN_TG(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_TG) == IEEE80211_CHAN_TG)

#define	IEEE80211_IS_CHAN_2GHZ(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_2GHZ) != 0)
#define	IEEE80211_IS_CHAN_5GHZ(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_5GHZ) != 0)
#define	IEEE80211_IS_CHAN_OFDM(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_OFDM) != 0)
#define	IEEE80211_IS_CHAN_CCK(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_CCK) != 0)
#define	IEEE80211_IS_CHAN_GFSK(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_GFSK) != 0)
#define	IEEE80211_IS_CHAN_XR(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_XR) != 0)

/* ni_chan encoding for FH phy */
#define	IEEE80211_FH_CHANMOD	80
#define	IEEE80211_FH_CHAN(set,pat)	(((set)-1)*IEEE80211_FH_CHANMOD+(pat))
#define	IEEE80211_FH_CHANSET(chan)	((chan)/IEEE80211_FH_CHANMOD+1)
#define	IEEE80211_FH_CHANPAT(chan)	((chan)%IEEE80211_FH_CHANMOD)

#define	IEEE80211_PS_SLEEP	0x1	/* STA is in power saving mode */

#define	IEEE80211_PS_MAX_QUEUE	50	/* maximum saved packets */

#pragma mark -
#pragma mark OpenBSD src/sys/net80211/ieee80211_proto.h

/*
 * Definitions ported from OpenBSD src/sys/net80211/ieee80211_proto.h r1.4
 */

enum ieee80211_state {
	IEEE80211_S_INIT	= 0,	/* default state */
	IEEE80211_S_SCAN	= 1,	/* scanning */
	IEEE80211_S_AUTH	= 2,	/* try to authenticate */
	IEEE80211_S_ASSOC	= 3,	/* try to assoc */
	IEEE80211_S_RUN		= 4	/* associated */
};
#define	IEEE80211_S_MAX		(IEEE80211_S_RUN+1)

#pragma mark -
#pragma mark OpenBSD src/sys/net80211/ieee80211_regdomain.h

/*
 * Definitions ported from OpenBSD src/sys/net80211/ieee80211_regdomain.h r1.8
 */

typedef u_int32_t ieee80211_regdomain_t;

enum ieee80211_regdomain {
	DMN_DEFAULT		= 0x00,
	DMN_NULL_WORLD		= 0x03,
	DMN_NULL_ETSIB		= 0x07,
	DMN_NULL_ETSIC		= 0x08,
	DMN_FCC1_FCCA		= 0x10,
	DMN_FCC1_WORLD		= 0x11,
	DMN_FCC2_FCCA		= 0x20,
	DMN_FCC2_WORLD		= 0x21,
	DMN_FCC2_ETSIC		= 0x22,
	DMN_FRANCE_NULL		= 0x31,
	DMN_FCC3_FCCA		= 0x3A,
	DMN_ETSI1_WORLD		= 0x37,
	DMN_ETSI3_ETSIA		= 0x32,
	DMN_ETSI2_WORLD		= 0x35,
	DMN_ETSI3_WORLD		= 0x36,
	DMN_ETSI4_WORLD		= 0x30,
	DMN_ETSI4_ETSIC		= 0x38,
	DMN_ETSI5_WORLD		= 0x39,
	DMN_ETSI6_WORLD		= 0x34,
	DMN_ETSI_NULL		= 0x33,
	DMN_MKK1_MKKA		= 0x40,
	DMN_MKK1_MKKB		= 0x41,
	DMN_APL4_WORLD		= 0x42,
	DMN_MKK2_MKKA		= 0x43,
	DMN_APL_NULL		= 0x44,
	DMN_APL2_WORLD		= 0x45,
	DMN_APL2_APLC		= 0x46,
	DMN_APL3_WORLD		= 0x47,
	DMN_MKK1_FCCA		= 0x48,
	DMN_APL2_APLD		= 0x49,
	DMN_MKK1_MKKA1		= 0x4A,
	DMN_MKK1_MKKA2		= 0x4B,
	DMN_APL1_WORLD		= 0x52,
	DMN_APL1_FCCA		= 0x53,
	DMN_APL1_APLA		= 0x54,
	DMN_APL1_ETSIC		= 0x55,
	DMN_APL2_ETSIC		= 0x56,
	DMN_APL5_WORLD		= 0x58,
	DMN_WOR0_WORLD		= 0x60,
	DMN_WOR1_WORLD		= 0x61,
	DMN_WOR2_WORLD		= 0x62,
	DMN_WOR3_WORLD		= 0x63,
	DMN_WOR4_WORLD		= 0x64,
	DMN_WOR5_ETSIC		= 0x65,
	DMN_WOR01_WORLD		= 0x66,
	DMN_WOR02_WORLD		= 0x67,
	DMN_EU1_WORLD		= 0x68,
	DMN_WOR9_WORLD		= 0x69,
	DMN_WORA_WORLD		= 0x6A,

	DMN_APL1		= 0xf0000001,
	DMN_APL2		= 0xf0000002,
	DMN_APL3		= 0xf0000004,
	DMN_APL4		= 0xf0000008,
	DMN_APL5		= 0xf0000010,
	DMN_ETSI1		= 0xf0000020,
	DMN_ETSI2		= 0xf0000040,
	DMN_ETSI3		= 0xf0000080,
	DMN_ETSI4		= 0xf0000100,
	DMN_ETSI5		= 0xf0000200,
	DMN_ETSI6		= 0xf0000400,
	DMN_ETSIA		= 0xf0000800,
	DMN_ETSIB		= 0xf0001000,
	DMN_ETSIC		= 0xf0002000,
	DMN_FCC1		= 0xf0004000,
	DMN_FCC2		= 0xf0008000,
	DMN_FCC3		= 0xf0010000,
	DMN_FCCA		= 0xf0020000,
	DMN_APLD		= 0xf0040000,
	DMN_MKK1		= 0xf0080000,
	DMN_MKK2		= 0xf0100000,
	DMN_MKKA		= 0xf0200000,
	DMN_NULL		= 0xf0400000,
	DMN_WORLD		= 0xf0800000,
	DMN_DEBUG               = 0xf1000000	/* used for debugging */
};

#define IEEE80211_DMN(_d)	((_d) & ~0xf0000000)

struct ieee80211_regdomainname {
	u_int32_t	rn_domain;
	const char	*rn_name;
};

#define IEEE80211_REGDOMAIN_NAMES {					\
	{ DMN_APL1,		    "APL1" },				\
	{ DMN_APL1_APLA,	    "APL1A" },				\
	{ DMN_APL1_ETSIC,	    "APL1_ETSIC" },			\
	{ DMN_APL1_FCCA,	    "APL1_FCCA" },			\
	{ DMN_APL1_WORLD,	    "APL1W" },				\
	{ DMN_APL2,		    "APL2" },				\
	{ DMN_APL2_APLC,	    "APL2C" },				\
	{ DMN_APL2_APLD,	    "APL2D" },				\
	{ DMN_APL2_ETSIC,	    "APL2_ETSIC" },			\
	{ DMN_APL2_WORLD,	    "APL2W" },				\
	{ DMN_APL3,		    "APL3" },				\
	{ DMN_APL3_WORLD,	    "APL3W" },				\
	{ DMN_APL4,		    "APL4" },				\
	{ DMN_APL4_WORLD,	    "APL4W" },				\
	{ DMN_APL5,		    "APL5" },				\
	{ DMN_APL5_WORLD,	    "APL5W" },				\
	{ DMN_APLD,		    "APLD" },				\
	{ DMN_APL_NULL,		    "APL" },				\
	{ DMN_DEBUG,		    "DEBUG" },				\
	{ DMN_ETSI1,		    "ETSI1" },				\
	{ DMN_ETSI1_WORLD,	    "ETSI1W" },				\
	{ DMN_ETSI2,		    "ETSI2" },				\
	{ DMN_ETSI2_WORLD,	    "ETSI2W" },				\
	{ DMN_ETSI3,		    "ETSI3" },				\
	{ DMN_ETSI3_ETSIA,	    "ETSI3A" },				\
	{ DMN_ETSI3_WORLD,	    "ETSI3W," },			\
	{ DMN_ETSI4,		    "ETSI4" },				\
	{ DMN_ETSI4_ETSIC,	    "ETSI4C" },				\
	{ DMN_ETSI4_WORLD,	    "ETSI4W" },				\
	{ DMN_ETSI5,		    "ETSI5" },				\
	{ DMN_ETSI5_WORLD,	    "ETSI5W" },				\
	{ DMN_ETSI6,		    "ETSI6" },				\
	{ DMN_ETSI6_WORLD,	    "ETSI6W" },				\
	{ DMN_ETSIA,		    "ETSIA" },				\
	{ DMN_ETSIB,		    "ETSIB" },				\
	{ DMN_ETSIC,		    "ETSIC" },				\
	{ DMN_ETSI_NULL,	    "ETSI" },				\
	{ DMN_EU1_WORLD,	    "EU1W" },				\
	{ DMN_FCC1,		    "FCC1" },				\
	{ DMN_FCC1_FCCA,	    "FCC1A" },				\
	{ DMN_FCC1_WORLD,	    "FCC1W" },				\
	{ DMN_FCC2,		    "FCC2" },				\
	{ DMN_FCC2_ETSIC,	    "FCC2C" },				\
	{ DMN_FCC2_FCCA,	    "FCC2A" },				\
	{ DMN_FCC2_WORLD,	    "FCC2W" },				\
	{ DMN_FCC3,		    "FCC3" },				\
	{ DMN_FCC3_FCCA,	    "FCC3A" },				\
	{ DMN_FCCA,		    "FCCA" },				\
	{ DMN_FRANCE_NULL,	    "FRANCE" },				\
	{ DMN_MKK1,		    "MKK1" },				\
	{ DMN_MKK1_FCCA,	    "MKK1_FCCA" },			\
	{ DMN_MKK1_MKKA,	    "MKK1A" },				\
	{ DMN_MKK1_MKKA1,	    "MKK1A1" },				\
	{ DMN_MKK1_MKKA2,	    "MKK1A2" },				\
	{ DMN_MKK1_MKKB,	    "MKK1B" },				\
	{ DMN_MKK2,		    "MKK2" },				\
	{ DMN_MKK2_MKKA,	    "MKK2A" },				\
	{ DMN_MKKA,		    "MKKA" },				\
	{ DMN_DEFAULT,		    "NONE" },				\
	{ DMN_NULL,		    "NONE" },				\
	{ DMN_NULL_ETSIB,	    "ETSIB" },				\
	{ DMN_NULL_ETSIC,	    "ETSIC" },				\
	{ DMN_WOR01_WORLD,	    "WOR01W" },				\
	{ DMN_WOR02_WORLD,	    "WOR02W" },				\
	{ DMN_WOR0_WORLD,	    "WOR0W" },				\
	{ DMN_WOR1_WORLD,	    "WOR1W" },				\
	{ DMN_WOR2_WORLD,	    "WOR2W" },				\
	{ DMN_WOR3_WORLD,	    "WOR3W" },				\
	{ DMN_WOR4_WORLD,	    "WOR4W" },				\
	{ DMN_WOR5_ETSIC,	    "WOR5_ETSIC" },			\
	{ DMN_WOR9_WORLD,	    "WOR9W" },				\
	{ DMN_WORA_WORLD,	    "WORAW" },				\
	{ DMN_NULL_WORLD,	    "WORLD" },				\
	{ DMN_WORLD,		    "WORLD" }				\
}

struct ieee80211_regdomainmap {
	u_int16_t	rm_domain;
	u_int32_t	rm_domain_5ghz;
	u_int32_t	rm_domain_2ghz;
};

#define IEEE80211_REGDOMAIN_MAP {					\
	{ DMN_DEFAULT,		DMN_DEBUG,	DMN_DEBUG },		\
	{ DMN_NULL_WORLD,	DMN_NULL,	DMN_WORLD },		\
	{ DMN_NULL_ETSIB,	DMN_NULL,	DMN_ETSIB },		\
	{ DMN_NULL_ETSIC,	DMN_NULL,	DMN_ETSIC },		\
	{ DMN_FCC1_FCCA,	DMN_FCC1,	DMN_FCCA },		\
	{ DMN_FCC1_WORLD,	DMN_FCC1,	DMN_WORLD },		\
	{ DMN_FCC2_FCCA,	DMN_FCC2,	DMN_FCCA },		\
	{ DMN_FCC2_WORLD,	DMN_FCC2,	DMN_WORLD },		\
	{ DMN_FCC2_ETSIC,	DMN_FCC2,	DMN_ETSIC },		\
	{ DMN_FRANCE_NULL,	DMN_ETSI3,	DMN_ETSI3 },		\
	{ DMN_FCC3_FCCA,	DMN_FCC3,	DMN_WORLD },		\
	{ DMN_ETSI1_WORLD,	DMN_ETSI1,	DMN_WORLD },		\
	{ DMN_ETSI3_ETSIA,	DMN_ETSI3,	DMN_WORLD },		\
	{ DMN_ETSI2_WORLD,	DMN_ETSI2,	DMN_WORLD },		\
	{ DMN_ETSI3_WORLD,	DMN_ETSI3,	DMN_WORLD },		\
	{ DMN_ETSI4_WORLD,	DMN_ETSI4,	DMN_WORLD },		\
	{ DMN_ETSI4_ETSIC,	DMN_ETSI4,	DMN_ETSIC },		\
	{ DMN_ETSI5_WORLD,	DMN_ETSI5,	DMN_WORLD },		\
	{ DMN_ETSI6_WORLD,	DMN_ETSI6,	DMN_WORLD },		\
	{ DMN_ETSI_NULL,	DMN_ETSI1,	DMN_ETSI1 },		\
	{ DMN_MKK1_MKKA,	DMN_MKK1,	DMN_MKKA },		\
	{ DMN_MKK1_MKKB,	DMN_MKK1,	DMN_MKKA },		\
	{ DMN_APL4_WORLD,	DMN_APL4,	DMN_WORLD },		\
	{ DMN_MKK2_MKKA,	DMN_MKK2,	DMN_MKKA },		\
	{ DMN_APL_NULL,		DMN_APL1,	DMN_NULL },		\
	{ DMN_APL2_WORLD,	DMN_APL2,	DMN_WORLD },		\
	{ DMN_APL2_APLC,	DMN_APL2,	DMN_WORLD },		\
	{ DMN_APL3_WORLD,	DMN_APL3,	DMN_WORLD },		\
	{ DMN_MKK1_FCCA,	DMN_MKK1,	DMN_FCCA },		\
	{ DMN_APL2_APLD,	DMN_APL2,	DMN_APLD },		\
	{ DMN_MKK1_MKKA1,	DMN_MKK1,	DMN_MKKA },		\
	{ DMN_MKK1_MKKA2,	DMN_MKK1,	DMN_MKKA },		\
	{ DMN_APL1_WORLD,	DMN_APL1,	DMN_WORLD },		\
	{ DMN_APL1_FCCA,	DMN_APL1,	DMN_FCCA },		\
	{ DMN_APL1_APLA,	DMN_APL1,	DMN_WORLD },		\
	{ DMN_APL1_ETSIC,	DMN_APL1,	DMN_ETSIC },		\
	{ DMN_APL2_ETSIC,	DMN_APL2,	DMN_ETSIC },		\
	{ DMN_APL5_WORLD,	DMN_APL5,	DMN_WORLD },		\
	{ DMN_WOR0_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR1_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR2_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR3_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR4_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR5_ETSIC,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR01_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WOR02_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_EU1_WORLD,	DMN_ETSI1,	DMN_WORLD },		\
	{ DMN_WOR9_WORLD,	DMN_WORLD,	DMN_WORLD },		\
	{ DMN_WORA_WORLD,	DMN_WORLD,	DMN_WORLD },		\
}

enum ieee80211_countrycode {
	CTRY_DEFAULT            = 0,   /* Default domain (NA) */
	CTRY_ALBANIA            = 8,   /* Albania */
	CTRY_ALGERIA            = 12,  /* Algeria */
	CTRY_ARGENTINA          = 32,  /* Argentina */
	CTRY_ARMENIA            = 51,  /* Armenia */
	CTRY_AUSTRALIA          = 36,  /* Australia */
	CTRY_AUSTRIA            = 40,  /* Austria */
	CTRY_AZERBAIJAN         = 31,  /* Azerbaijan */
	CTRY_BAHRAIN            = 48,  /* Bahrain */
	CTRY_BELARUS            = 112, /* Belarus */
	CTRY_BELGIUM            = 56,  /* Belgium */
	CTRY_BELIZE             = 84,  /* Belize */
	CTRY_BOLIVIA            = 68,  /* Bolivia */
	CTRY_BRAZIL             = 76,  /* Brazil */
	CTRY_BRUNEI_DARUSSALAM  = 96,  /* Brunei Darussalam */
	CTRY_BULGARIA           = 100, /* Bulgaria */
	CTRY_CANADA             = 124, /* Canada */
	CTRY_CHILE              = 152, /* Chile */
	CTRY_CHINA              = 156, /* People's Republic of China */
	CTRY_COLOMBIA           = 170, /* Colombia */
	CTRY_COSTA_RICA         = 188, /* Costa Rica */
	CTRY_CROATIA            = 191, /* Croatia */
	CTRY_CYPRUS             = 196, /* Cyprus */
	CTRY_CZECH              = 203, /* Czech Republic */
	CTRY_DENMARK            = 208, /* Denmark */
	CTRY_DOMINICAN_REPUBLIC = 214, /* Dominican Republic */
	CTRY_ECUADOR            = 218, /* Ecuador */
	CTRY_EGYPT              = 818, /* Egypt */
	CTRY_EL_SALVADOR        = 222, /* El Salvador */
	CTRY_ESTONIA            = 233, /* Estonia */
	CTRY_FAEROE_ISLANDS     = 234, /* Faeroe Islands */
	CTRY_FINLAND            = 246, /* Finland */
	CTRY_FRANCE             = 250, /* France */
	CTRY_FRANCE2            = 255, /* France2 */
	CTRY_GEORGIA            = 268, /* Georgia */
	CTRY_GERMANY            = 276, /* Germany */
	CTRY_GREECE             = 300, /* Greece */
	CTRY_GUATEMALA          = 320, /* Guatemala */
	CTRY_HONDURAS           = 340, /* Honduras */
	CTRY_HONG_KONG          = 344, /* Hong Kong S.A.R., P.R.C. */
	CTRY_HUNGARY            = 348, /* Hungary */
	CTRY_ICELAND            = 352, /* Iceland */
	CTRY_INDIA              = 356, /* India */
	CTRY_INDONESIA          = 360, /* Indonesia */
	CTRY_IRAN               = 364, /* Iran */
	CTRY_IRAQ               = 368, /* Iraq */
	CTRY_IRELAND            = 372, /* Ireland */
	CTRY_ISRAEL             = 376, /* Israel */
	CTRY_ITALY              = 380, /* Italy */
	CTRY_JAMAICA            = 388, /* Jamaica */
	CTRY_JAPAN              = 392, /* Japan */
	CTRY_JAPAN1             = 393, /* Japan (JP1) */
	CTRY_JAPAN2             = 394, /* Japan (JP0) */
	CTRY_JAPAN3             = 395, /* Japan (JP1-1) */
	CTRY_JAPAN4             = 396, /* Japan (JE1) */
	CTRY_JAPAN5             = 397, /* Japan (JE2) */
	CTRY_JORDAN             = 400, /* Jordan */
	CTRY_KAZAKHSTAN         = 398, /* Kazakhstan */
	CTRY_KENYA              = 404, /* Kenya */
	CTRY_KOREA_NORTH        = 408, /* North Korea */
	CTRY_KOREA_ROC          = 410, /* South Korea */
	CTRY_KOREA_ROC2         = 411, /* South Korea */
	CTRY_KUWAIT             = 414, /* Kuwait */
	CTRY_LATVIA             = 428, /* Latvia */
	CTRY_LEBANON            = 422, /* Lebanon */
	CTRY_LIBYA              = 434, /* Libya */
	CTRY_LIECHTENSTEIN      = 438, /* Liechtenstein */
	CTRY_LITHUANIA          = 440, /* Lithuania */
	CTRY_LUXEMBOURG         = 442, /* Luxembourg */
	CTRY_MACAU              = 446, /* Macau */
	CTRY_MACEDONIA          = 807, /* Republic of Macedonia */
	CTRY_MALAYSIA           = 458, /* Malaysia */
	CTRY_MEXICO             = 484, /* Mexico */
	CTRY_MONACO             = 492, /* Principality of Monaco */
	CTRY_MOROCCO            = 504, /* Morocco */
	CTRY_NETHERLANDS        = 528, /* Netherlands */
	CTRY_NEW_ZEALAND        = 554, /* New Zealand */
	CTRY_NICARAGUA          = 558, /* Nicaragua */
	CTRY_NORWAY             = 578, /* Norway */
	CTRY_OMAN               = 512, /* Oman */
	CTRY_PAKISTAN           = 586, /* Islamic Republic of Pakistan */
	CTRY_PANAMA             = 591, /* Panama */
	CTRY_PARAGUAY           = 600, /* Paraguay */
	CTRY_PERU               = 604, /* Peru */
	CTRY_PHILIPPINES        = 608, /* Republic of the Philippines */
	CTRY_POLAND             = 616, /* Poland */
	CTRY_PORTUGAL           = 620, /* Portugal */
	CTRY_PUERTO_RICO        = 630, /* Puerto Rico */
	CTRY_QATAR              = 634, /* Qatar */
	CTRY_ROMANIA            = 642, /* Romania */
	CTRY_RUSSIA             = 643, /* Russia */
	CTRY_SAUDI_ARABIA       = 682, /* Saudi Arabia */
	CTRY_SINGAPORE          = 702, /* Singapore */
	CTRY_SLOVAKIA           = 703, /* Slovak Republic */
	CTRY_SLOVENIA           = 705, /* Slovenia */
	CTRY_SOUTH_AFRICA       = 710, /* South Africa */
	CTRY_SPAIN              = 724, /* Spain */
	CTRY_SRI_LANKA          = 728, /* Sri Lanka */
	CTRY_SWEDEN             = 752, /* Sweden */
	CTRY_SWITZERLAND        = 756, /* Switzerland */
	CTRY_SYRIA              = 760, /* Syria */
	CTRY_TAIWAN             = 158, /* Taiwan */
	CTRY_THAILAND           = 764, /* Thailand */
	CTRY_TRINIDAD_Y_TOBAGO  = 780, /* Trinidad y Tobago */
	CTRY_TUNISIA            = 788, /* Tunisia */
	CTRY_TURKEY             = 792, /* Turkey */
	CTRY_UAE                = 784, /* U.A.E. */
	CTRY_UKRAINE            = 804, /* Ukraine */
	CTRY_UNITED_KINGDOM     = 826, /* United Kingdom */
	CTRY_UNITED_STATES      = 840, /* United States */
	CTRY_URUGUAY            = 858, /* Uruguay */
	CTRY_UZBEKISTAN         = 860, /* Uzbekistan */
	CTRY_VENEZUELA          = 862, /* Venezuela */
	CTRY_VIET_NAM           = 704, /* Viet Nam */
	CTRY_YEMEN              = 887, /* Yemen */
	CTRY_ZIMBABWE           = 716, /* Zimbabwe */
};

struct ieee80211_countryname {
	u_int16_t	cn_code;
	const char	*cn_name;
	u_int32_t	cn_domain;
};

#define IEEE80211_REGDOMAIN_COUNTRY_NAMES {				\
	{ CTRY_DEFAULT,            "00", DMN_DEFAULT },			\
	{ CTRY_UAE,                "ae", DMN_NULL_WORLD },		\
	{ CTRY_ALBANIA,            "al", DMN_NULL_WORLD },		\
	{ CTRY_ARMENIA,            "am", DMN_ETSI4_WORLD },		\
	{ CTRY_ARGENTINA,          "ar", DMN_APL3_WORLD },		\
	{ CTRY_AUSTRIA,            "at", DMN_ETSI5_WORLD },		\
	{ CTRY_AUSTRALIA,          "au", DMN_FCC2_WORLD },		\
	{ CTRY_AZERBAIJAN,         "az", DMN_ETSI4_WORLD },		\
	{ CTRY_BELGIUM,            "be", DMN_ETSI4_WORLD },		\
	{ CTRY_BULGARIA,           "bg", DMN_ETSI6_WORLD },		\
	{ CTRY_BAHRAIN,            "bh", DMN_NULL_WORLD },		\
	{ CTRY_BRUNEI_DARUSSALAM,  "bn", DMN_APL1_WORLD },		\
	{ CTRY_BOLIVIA,            "bo", DMN_APL1_ETSIC },		\
	{ CTRY_BRAZIL,             "br", DMN_NULL_ETSIC },		\
	{ CTRY_BELARUS,            "by", DMN_NULL_WORLD },		\
	{ CTRY_BELIZE,             "bz", DMN_NULL_ETSIC },		\
	{ CTRY_CANADA,             "ca", DMN_FCC2_FCCA },		\
	{ CTRY_SWITZERLAND,        "ch", DMN_ETSI2_WORLD },		\
	{ CTRY_CHILE,              "cl", DMN_APL5_WORLD },		\
	{ CTRY_CHINA,              "cn", DMN_APL1_WORLD },		\
	{ CTRY_COLOMBIA,           "co", DMN_FCC1_FCCA },		\
	{ CTRY_COSTA_RICA,         "cr", DMN_NULL_WORLD },		\
	{ CTRY_CYPRUS,             "cy", DMN_ETSI1_WORLD },		\
	{ CTRY_CZECH,              "cz", DMN_ETSI3_WORLD },		\
	{ CTRY_GERMANY,            "de", DMN_ETSI1_WORLD },		\
	{ CTRY_DENMARK,            "dk", DMN_ETSI1_WORLD },		\
	{ CTRY_DOMINICAN_REPUBLIC, "do", DMN_FCC1_FCCA },		\
	{ CTRY_ALGERIA,            "dz", DMN_NULL_WORLD },		\
	{ CTRY_ECUADOR,            "ec", DMN_NULL_WORLD },		\
	{ CTRY_ESTONIA,            "ee", DMN_ETSI1_WORLD },		\
	{ CTRY_EGYPT,              "eg", DMN_NULL_WORLD },		\
	{ CTRY_SPAIN,              "es", DMN_ETSI1_WORLD },		\
	{ CTRY_FRANCE2,            "f2", DMN_ETSI3_WORLD },		\
	{ CTRY_FINLAND,            "fi", DMN_ETSI1_WORLD },		\
	{ CTRY_FAEROE_ISLANDS,     "fo", DMN_NULL_WORLD },		\
	{ CTRY_FRANCE,             "fr", DMN_ETSI3_WORLD },		\
	{ CTRY_GEORGIA,            "ge", DMN_ETSI4_WORLD },		\
	{ CTRY_GREECE,             "gr", DMN_NULL_WORLD },		\
	{ CTRY_GUATEMALA,          "gt", DMN_FCC1_FCCA },		\
	{ CTRY_HONG_KONG,          "hk", DMN_FCC2_WORLD },		\
	{ CTRY_HONDURAS,           "hn", DMN_NULL_WORLD },		\
	{ CTRY_CROATIA,            "hr", DMN_ETSI3_WORLD },		\
	{ CTRY_HUNGARY,            "hu", DMN_ETSI2_WORLD },		\
	{ CTRY_INDONESIA,          "id", DMN_NULL_WORLD },		\
	{ CTRY_IRELAND,            "ie", DMN_ETSI1_WORLD },		\
	{ CTRY_ISRAEL,             "il", DMN_NULL_WORLD },		\
	{ CTRY_INDIA,              "in", DMN_NULL_WORLD },		\
	{ CTRY_IRAQ,               "iq", DMN_NULL_WORLD },		\
	{ CTRY_IRAN,               "ir", DMN_APL1_WORLD },		\
	{ CTRY_ICELAND,            "is", DMN_ETSI1_WORLD },		\
	{ CTRY_ITALY,              "it", DMN_ETSI1_WORLD },		\
	{ CTRY_JAPAN1,             "j1", DMN_MKK1_MKKB },		\
	{ CTRY_JAPAN2,             "j2", DMN_MKK1_FCCA },		\
	{ CTRY_JAPAN3,             "j3", DMN_MKK2_MKKA },		\
	{ CTRY_JAPAN4,             "j4", DMN_MKK1_MKKA1 },		\
	{ CTRY_JAPAN5,             "j5", DMN_MKK1_MKKA2 },		\
	{ CTRY_JAMAICA,            "jm", DMN_NULL_WORLD },		\
	{ CTRY_JORDAN,             "jo", DMN_NULL_WORLD },		\
	{ CTRY_JAPAN,              "jp", DMN_MKK1_MKKA },		\
	{ CTRY_KOREA_ROC2,         "k2", DMN_APL2_APLD },		\
	{ CTRY_KENYA,              "ke", DMN_NULL_WORLD },		\
	{ CTRY_KOREA_NORTH,        "kp", DMN_APL2_WORLD },		\
	{ CTRY_KOREA_ROC,          "kr", DMN_APL2_WORLD },		\
	{ CTRY_KUWAIT,             "kw", DMN_NULL_WORLD },		\
	{ CTRY_KAZAKHSTAN,         "kz", DMN_NULL_WORLD },		\
	{ CTRY_LEBANON,            "lb", DMN_NULL_WORLD },		\
	{ CTRY_LIECHTENSTEIN,      "li", DMN_ETSI2_WORLD },		\
	{ CTRY_SRI_LANKA,          "lk", DMN_NULL_WORLD },		\
	{ CTRY_LITHUANIA,          "lt", DMN_ETSI1_WORLD },		\
	{ CTRY_LUXEMBOURG,         "lu", DMN_ETSI1_WORLD },		\
	{ CTRY_LATVIA,             "lv", DMN_NULL_WORLD },		\
	{ CTRY_LIBYA,              "ly", DMN_NULL_WORLD },		\
	{ CTRY_MOROCCO,            "ma", DMN_NULL_WORLD },		\
	{ CTRY_MONACO,             "mc", DMN_ETSI4_WORLD },		\
	{ CTRY_MACEDONIA,          "mk", DMN_NULL_WORLD },		\
	{ CTRY_MACAU,              "mo", DMN_FCC2_WORLD },		\
	{ CTRY_MEXICO,             "mx", DMN_FCC1_FCCA },		\
	{ CTRY_MALAYSIA,           "my", DMN_NULL_WORLD },		\
	{ CTRY_NICARAGUA,          "ni", DMN_NULL_WORLD },		\
	{ CTRY_NETHERLANDS,        "nl", DMN_ETSI1_WORLD },		\
	{ CTRY_NORWAY,             "no", DMN_ETSI1_WORLD },		\
	{ CTRY_NEW_ZEALAND,        "nz", DMN_FCC2_ETSIC },		\
	{ CTRY_OMAN,               "om", DMN_NULL_WORLD },		\
	{ CTRY_PANAMA,             "pa", DMN_FCC1_FCCA },		\
	{ CTRY_PERU,               "pe", DMN_NULL_WORLD },		\
	{ CTRY_PHILIPPINES,        "ph", DMN_FCC1_WORLD },		\
	{ CTRY_PAKISTAN,           "pk", DMN_NULL_WORLD },		\
	{ CTRY_POLAND,             "pl", DMN_ETSI1_WORLD },		\
	{ CTRY_PUERTO_RICO,        "pr", DMN_FCC1_FCCA },		\
	{ CTRY_PORTUGAL,           "pt", DMN_ETSI1_WORLD },		\
	{ CTRY_PARAGUAY,           "py", DMN_NULL_WORLD },		\
	{ CTRY_QATAR,              "qa", DMN_NULL_WORLD },		\
	{ CTRY_ROMANIA,            "ro", DMN_NULL_WORLD },		\
	{ CTRY_RUSSIA,             "ru", DMN_NULL_WORLD },		\
	{ CTRY_SAUDI_ARABIA,       "sa", DMN_NULL_WORLD },		\
	{ CTRY_SWEDEN,             "se", DMN_ETSI1_WORLD },		\
	{ CTRY_SINGAPORE,          "sg", DMN_APL4_WORLD },		\
	{ CTRY_SLOVENIA,           "si", DMN_ETSI1_WORLD },		\
	{ CTRY_SLOVAKIA,           "sk", DMN_ETSI3_WORLD },		\
	{ CTRY_EL_SALVADOR,        "sv", DMN_NULL_WORLD },		\
	{ CTRY_SYRIA,              "sy", DMN_NULL_WORLD },		\
	{ CTRY_THAILAND,           "th", DMN_APL2_WORLD },		\
	{ CTRY_TUNISIA,            "tn", DMN_ETSI3_WORLD },		\
	{ CTRY_TURKEY,             "tr", DMN_ETSI3_WORLD },		\
	{ CTRY_TRINIDAD_Y_TOBAGO,  "tt", DMN_ETSI4_WORLD },		\
	{ CTRY_TAIWAN,             "tw", DMN_APL3_WORLD },		\
	{ CTRY_UKRAINE,            "ua", DMN_NULL_WORLD },		\
	{ CTRY_UNITED_KINGDOM,     "uk", DMN_ETSI1_WORLD },		\
	{ CTRY_UNITED_STATES,      "us", DMN_FCC1_FCCA },		\
	{ CTRY_URUGUAY,            "uy", DMN_APL2_WORLD },		\
	{ CTRY_UZBEKISTAN,         "uz", DMN_FCC3_FCCA },		\
	{ CTRY_VENEZUELA,          "ve", DMN_APL2_ETSIC },		\
	{ CTRY_VIET_NAM,           "vn", DMN_NULL_WORLD },		\
	{ CTRY_YEMEN,              "ye", DMN_NULL_WORLD },		\
	{ CTRY_SOUTH_AFRICA,       "za", DMN_ETSI1_WORLD },		\
	{ CTRY_ZIMBABWE,           "zw", DMN_NULL_WORLD },		\
}

enum ieee80211_ctl {
	CTL_11A		= 0x00,
	CTL_11B		= 0x01,
	CTL_11G		= 0x02,
	CTL_TURBO	= 0x03,
	CTL_TURBO_G	= 0x04,
	CTL_FCC		= 0x10,
	CTL_ETSI	= 0x30,
	CTL_MKK		= 0x40,
	CTL_NONE	= 0xff
};

#define IEEE80211_CHANNELS_2GHZ_MIN	2412	/* 2GHz channel 1 */
#define IEEE80211_CHANNELS_2GHZ_MAX	2732	/* 2GHz channel 26 */

struct ieee80211_regchannel {
	u_int16_t	rc_channel;
	u_int32_t	rc_domain;
	u_int32_t	rc_mode;
};

#define IEEE80211_CHANNELS_2GHZ {					\
	{ 2412, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2417, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2422, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2427, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2432, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2442, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2447, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2452, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2457, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2462, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2467, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2472, DMN_APLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
									\
	{ 2432, DMN_ETSIB, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_ETSIB, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|	\
	    IEEE80211_CHAN_TURBO },					\
	{ 2442, DMN_ETSIB, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
									\
	{ 2412, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2417, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2422, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2427, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2432, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|	\
	    IEEE80211_CHAN_TURBO },					\
	{ 2442, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2447, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2452, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2457, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2462, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2467, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2472, DMN_ETSIC, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
									\
	{ 2412, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2417, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2422, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2427, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2432, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|	\
	    IEEE80211_CHAN_TURBO },					\
	{ 2442, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2447, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2452, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2457, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2462, DMN_FCCA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
									\
	{ 2412, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2417, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2422, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2427, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2432, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2442, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2447, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2452, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2457, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2462, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2467, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2472, DMN_MKKA, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2484, DMN_MKKA, IEEE80211_CHAN_CCK },				\
									\
	{ 2412, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2417, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2422, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2427, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2432, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2437, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|	\
	    IEEE80211_CHAN_TURBO },					\
	{ 2442, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2447, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2452, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2457, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2462, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2467, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
	{ 2472, DMN_WORLD, IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM },	\
}

#define IEEE80211_CHANNELS_5GHZ_MIN	5005	/* 5GHz channel 1 */
#define IEEE80211_CHANNELS_5GHZ_MAX	6100	/* 5GHz channel 220 */

#define IEEE80211_CHANNELS_5GHZ {					\
	{ 5745, DMN_APL1, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_APL1, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_APL1, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_APL1, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_APL1, IEEE80211_CHAN_OFDM },			\
									\
	{ 5745, DMN_APL2, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_APL2, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_APL2, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_APL2, IEEE80211_CHAN_OFDM },			\
									\
	{ 5280, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5300, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5745, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_APL3, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_APL3, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5745, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_APL4, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_APL4, IEEE80211_CHAN_OFDM },			\
									\
	{ 5745, DMN_APL5, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_APL5, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_APL5, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_APL5, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_APL5, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5260, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5300, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5500, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5520, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5540, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5560, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5580, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5600, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5620, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5640, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5660, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5680, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
	{ 5700, DMN_ETSI1, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI2, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI2, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI2, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI2, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5260, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5300, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_ETSI3, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5260, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5300, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_ETSI4, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI5, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI5, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI5, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI5, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5260, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5500, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5520, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5540, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5560, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5580, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5600, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5620, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5640, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5660, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5680, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
	{ 5700, DMN_ETSI6, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5210, DMN_FCC1, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5220, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5250, DMN_FCC1, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5260, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5290, DMN_FCC1, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5300, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5745, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5760, DMN_FCC1, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5765, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5800, DMN_FCC1, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5805, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_FCC1, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5260, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5300, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5745, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5765, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5805, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_FCC2, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5210, DMN_FCC3, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5220, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5250, DMN_FCC3, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5260, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5280, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5290, DMN_FCC3, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5300, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5320, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5500, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5520, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5540, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5560, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5580, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5600, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5620, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5640, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5660, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5680, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5700, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5745, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5760, DMN_FCC3, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5765, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5785, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5800, DMN_FCC3, IEEE80211_CHAN_OFDM|IEEE80211_CHAN_TURBO },	\
	{ 5805, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
	{ 5825, DMN_FCC3, IEEE80211_CHAN_OFDM },			\
									\
	{ 5170, DMN_MKK1, IEEE80211_CHAN_OFDM },			\
	{ 5190, DMN_MKK1, IEEE80211_CHAN_OFDM },			\
	{ 5210, DMN_MKK1, IEEE80211_CHAN_OFDM },			\
	{ 5230, DMN_MKK1, IEEE80211_CHAN_OFDM },			\
									\
	{ 5040, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5060, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5080, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5170, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5190, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5210, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
	{ 5230, DMN_MKK2, IEEE80211_CHAN_OFDM },			\
									\
	{ 5180, DMN_WORLD, IEEE80211_CHAN_OFDM },			\
	{ 5200, DMN_WORLD, IEEE80211_CHAN_OFDM },			\
	{ 5220, DMN_WORLD, IEEE80211_CHAN_OFDM },			\
	{ 5240, DMN_WORLD, IEEE80211_CHAN_OFDM },			\
}

#endif /* IEEELITE */
