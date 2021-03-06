/*
 *  OpenHAL5212.cpp
 *  AtheroJack
 *
 *  The code in this file is based on the code in the OpenBSD file
 *  sys/dev/ic/ar5212.c r1.28.
 *
 *  Ported by Michael Rossberg, Beat Zahnd
 *
 */

/*
 * Copyright (c) 2004, 2005 Reyk Floeter <reyk@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * HAL interface for the Atheros AR5001 Wireless LAN chipset
 * (AR5212 + AR5111/AR5112).
 */

#include "OpenHAL5212.h"
#include "ar5212var.h"
#include "ar5212reg.h"
#include "../WiFiLogger.h"

extern "C" {
#include <sys/param.h>
}

static const u_char etherbroadcastaddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

OSDefineMetaClassAndStructors(OpenHAL5212, OpenHAL);

#pragma mark -

/*
 * Initial register setting for the AR5212
 */
static const struct ar5k_ar5212_ini ar5212_ini[] =
    AR5K_AR5212_INI;
static const struct ar5k_ar5212_ini_mode ar5212_mode[] =
    AR5K_AR5212_INI_MODE;

const void
OpenHAL5212::ar5k_ar5212_fill()
{
	ah_magic = AR5K_AR5212_MAGIC;
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_attach(u_int16_t device, bus_space_tag_t st,
    bus_space_handle_t sh, HAL_STATUS *status)
{
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int32_t srev;

	ar5k_ar5212_fill();

	/* Bring device out of sleep and reset it's units */
	if (ar5k_ar5212_nic_wakeup(AR5K_INIT_MODE) != AH_TRUE)
		return (AH_FALSE);

	/* Get MAC, PHY and RADIO revisions */
	srev = AR5K_REG_READ(AR5K_AR5212_SREV);
	ah_mac_srev = srev;
	ah_mac_version = AR5K_REG_MS(srev, AR5K_AR5212_SREV_VER);
	ah_mac_revision = AR5K_REG_MS(srev, AR5K_AR5212_SREV_REV);
	ah_phy_revision = AR5K_REG_READ(AR5K_AR5212_PHY_CHIP_ID) &
	    0x00ffffffff;
	ah_radio_5ghz_revision =
	    ar5k_ar5212_radio_revision(HAL_CHIP_5GHZ);
	ah_radio_2ghz_revision =
	    ar5k_ar5212_radio_revision(HAL_CHIP_2GHZ);

	/* Single chip radio */
	if (ah_radio_2ghz_revision == ah_radio_5ghz_revision)
		ah_radio_2ghz_revision = 0;

	/* Identify the chipset (this has to be done in an early step) */
	ah_version = AR5K_AR5212;
	ah_radio = ah_radio_5ghz_revision < AR5K_SREV_RAD_5112 ?
	    AR5K_AR5111 : AR5K_AR5112;
	ah_phy = AR5K_AR5212_PHY(0);

	bcopy(etherbroadcastaddr, mac, IEEE80211_ADDR_LEN);
	ar5k_ar5212_set_associd(mac, 0, 0);
	ar5k_ar5212_get_lladdr(mac);
	ar5k_ar5212_set_opmode();

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_nic_reset(u_int32_t val)
{
	HAL_BOOL ret = AH_FALSE;
	u_int32_t mask = val ? val : ~0;

	/* Read-and-clear */
	AR5K_REG_READ(AR5K_AR5212_RXDP);

	/*
	 * Reset the device and wait until success
	 */
	AR5K_REG_WRITE(AR5K_AR5212_RC, val);

	/* Wait at least 128 PCI clocks */
	AR5K_DELAY(15);

	val &=
	    AR5K_AR5212_RC_PCU | AR5K_AR5212_RC_BB;

	mask &=
	    AR5K_AR5212_RC_PCU | AR5K_AR5212_RC_BB;

	ret = ar5k_register_timeout(AR5K_AR5212_RC, mask, val, AH_FALSE);

	/*
	 * Reset configuration register
	 */
	if ((val & AR5K_AR5212_RC_PCU) == 0)
		AR5K_REG_WRITE(AR5K_AR5212_CFG, AR5K_AR5212_INIT_CFG);

	return (ret);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_nic_wakeup(u_int16_t flags)
{
	u_int32_t turbo, mode, clock;

	turbo = 0;
	mode = 0;
	clock = 0;

	/*
	 * Get channel mode flags
	 */

	if (ah_radio >= AR5K_AR5112) {
		mode = AR5K_AR5212_PHY_MODE_RAD_AR5112;
		clock = AR5K_AR5212_PHY_PLL_AR5112;
	} else {
		mode = AR5K_AR5212_PHY_MODE_RAD_AR5111;
		clock = AR5K_AR5212_PHY_PLL_AR5111;
	}

	if (flags & IEEE80211_CHAN_2GHZ) {
		mode |= AR5K_AR5212_PHY_MODE_FREQ_2GHZ;
		clock |= AR5K_AR5212_PHY_PLL_44MHZ;
	} else if (flags & IEEE80211_CHAN_5GHZ) {
		mode |= AR5K_AR5212_PHY_MODE_FREQ_5GHZ;
		clock |= AR5K_AR5212_PHY_PLL_40MHZ;
	} else {
		AR5K_PRINT("invalid radio frequency mode\n");
		return (AH_FALSE);
	}

	if (flags & IEEE80211_CHAN_CCK) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_CCK;
	} else if (flags & IEEE80211_CHAN_OFDM) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_OFDM;
	} else if (flags & IEEE80211_CHAN_DYN) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_DYN;
	} else {
		AR5K_PRINT("invalid radio frequency mode\n");
		return (AH_FALSE);
	}

	if (flags & IEEE80211_CHAN_TURBO) {
		turbo = AR5K_AR5212_PHY_TURBO_MODE |
		    AR5K_AR5212_PHY_TURBO_SHORT;
	}

	/*
	 * Reset and wakeup the device
	 */

	/* ...reset chipset and PCI device */
	if (ar5k_ar5212_nic_reset(
		AR5K_AR5212_RC_CHIP | AR5K_AR5212_RC_PCI) == AH_FALSE) {
		AR5K_PRINT("failed to reset the AR5212 + PCI chipset\n");
		return (AH_FALSE);
	}

	/* ...wakeup */
	if (ar5k_ar5212_set_power(
		HAL_PM_AWAKE, AH_TRUE, 0) == AH_FALSE) {
		AR5K_PRINT("failed to resume the AR5212 (again)\n");
		return (AH_FALSE);
	}

	/* ...final warm reset */
	if (ar5k_ar5212_nic_reset(0) == AH_FALSE) {
		AR5K_PRINT("failed to warm reset the AR5212\n");
		return (AH_FALSE);
	}

	/* ...set the PHY operating mode */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_PLL, clock);
	AR5K_DELAY(300);

	AR5K_REG_WRITE(AR5K_AR5212_PHY_MODE, mode);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_TURBO, turbo);

	return (AH_TRUE);
}

u_int16_t
OpenHAL5212::ar5k_ar5212_radio_revision(HAL_CHIP chip)
{
	int i;
	u_int32_t srev;
	u_int16_t ret;

	/*
	 * Set the radio chip access register
	 */
	switch (chip) {
	case HAL_CHIP_2GHZ:
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_2GHZ);
		break;
	case HAL_CHIP_5GHZ:
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);
		break;
	default:
		return (0);
	}

	AR5K_DELAY(2000);

	/* ...wait until PHY is ready and read the selected radio revision */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x34), 0x00001c16);

	for (i = 0; i < 8; i++)
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0x20), 0x00010000);
	srev = (AR5K_REG_READ(AR5K_AR5212_PHY(0x100)) >> 24) & 0xff;

	ret = ar5k_bitswap(((srev & 0xf0) >> 4) | ((srev & 0x0f) << 4), 8);

	/* Reset to the 5GHz mode */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);

	return (ret);
}

const HAL_RATE_TABLE *
OpenHAL5212::ar5k_ar5212_get_rate_table(u_int mode)
{
	switch (mode) {
	case HAL_MODE_11A:
		return (&ah_rt_11a);
	case HAL_MODE_TURBO:
		return (&ah_rt_turbo);
	case HAL_MODE_11B:
		return (&ah_rt_11b);
	case HAL_MODE_11G:
	case HAL_MODE_PUREG:
		return (&ah_rt_11g);
	case HAL_MODE_XR:
		return (&ah_rt_xr);
	default:
		return (NULL);
	}

	return (NULL);
}

void
OpenHAL5212::ar5k_ar5212_detach()
{
	if (ah_rf_banks != NULL)
		IOFree(ah_rf_banks, ah_rf_banks_size);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_reset(HAL_OPMODE op_mode, HAL_CHANNEL *channel,
    HAL_BOOL change_channel, HAL_STATUS *status)
{
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int32_t data, s_seq, s_ant, s_led[3];
	u_int i, phy, mode, freq, off, ee_mode, ant[2];
	const HAL_RATE_TABLE *rt;

	/*
	 * Save some registers before a reset
	 */
	if (change_channel == AH_TRUE) {
		s_seq = AR5K_REG_READ(AR5K_AR5212_DCU_SEQNUM(0));
		s_ant = AR5K_REG_READ(AR5K_AR5212_DEFAULT_ANTENNA);
	} else {
		s_seq = 0;
		s_ant = 1;
	}

	s_led[0] = AR5K_REG_READ(AR5K_AR5212_PCICFG) &
	    AR5K_AR5212_PCICFG_LEDSTATE;
	s_led[1] = AR5K_REG_READ(AR5K_AR5212_GPIOCR);
	s_led[2] = AR5K_REG_READ(AR5K_AR5212_GPIODO);

	if (change_channel == AH_TRUE && ah_rf_banks != NULL)
		ar5k_ar5212_get_rf_gain();

	if (ar5k_ar5212_nic_wakeup(channel->c_channel_flags) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Initialize operating mode
	 */
	ah_op_mode = op_mode;

	if (ah_radio == AR5K_AR5111) {
		phy = AR5K_INI_PHY_5111;
	} else if (ah_radio == AR5K_AR5112) {
		phy = AR5K_INI_PHY_5112;
	} else {
		AR5K_PRINTF("invalid phy radio: %u\n", ah_radio);
		return (AH_FALSE);
	}

	if (channel->c_channel_flags & IEEE80211_CHAN_A) {
		mode = AR5K_INI_VAL_11A;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_T) {
		mode = AR5K_INI_VAL_11A_TURBO;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_B) {
		mode = AR5K_INI_VAL_11B;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11B;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_G) {
		mode = AR5K_INI_VAL_11G;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11G;
	} else if (channel->c_channel_flags & CHANNEL_TG) {
		mode = AR5K_INI_VAL_11G_TURBO;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11G;
	} else if (channel->c_channel_flags & CHANNEL_XR) {
		mode = AR5K_INI_VAL_XR;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else {
		AR5K_PRINTF("invalid channel: %d\n", channel->c_channel);
		return (AH_FALSE);
	}

	/* PHY access enable */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);

	/*
	 * Write initial mode settings
	 */
	for (i = 0; i < AR5K_ELEMENTS(ar5212_mode); i++) {
		if (ar5212_mode[i].mode_flags == AR5K_INI_FLAG_511X)
			off = AR5K_INI_PHY_511X;
		else if (ar5212_mode[i].mode_flags & AR5K_INI_FLAG_5111 &&
		    ah_radio == AR5K_AR5111)
			off = AR5K_INI_PHY_5111;
		else if (ar5212_mode[i].mode_flags & AR5K_INI_FLAG_5112 &&
		    ah_radio == AR5K_AR5112)
			off = AR5K_INI_PHY_5112;
		else
			continue;

		AR5K_REG_WAIT(i);
		AR5K_REG_WRITE((u_int32_t)ar5212_mode[i].mode_register,
		    ar5212_mode[i].mode_value[off][mode]);
	}

	/*
	 * Write initial register settings
	 */
	for (i = 0; i < AR5K_ELEMENTS(ar5212_ini); i++) {
		if (change_channel == AH_TRUE &&
		    ar5212_ini[i].ini_register >= AR5K_AR5212_PCU_MIN &&
		    ar5212_ini[i].ini_register <= AR5K_AR5212_PCU_MAX)
			continue;

		if ((ah_radio == AR5K_AR5111 &&
		    ar5212_ini[i].ini_flags & AR5K_INI_FLAG_5111) ||
		    (ah_radio == AR5K_AR5112 &&
		    ar5212_ini[i].ini_flags & AR5K_INI_FLAG_5112)) {
			AR5K_REG_WAIT(i);
			AR5K_REG_WRITE((u_int32_t)ar5212_ini[i].ini_register,
			    ar5212_ini[i].ini_value);
		}
	}

	/*
	 * Write initial RF gain settings
	 */
	if (ar5k_rfgain(phy, freq) == AH_FALSE)
		return (AH_FALSE);

	AR5K_DELAY(1000);

	/*
	 * Set rate duration table
	 */
	rt = ar5k_ar5212_get_rate_table(
	    channel->c_channel_flags & IEEE80211_CHAN_TURBO ?
	    HAL_MODE_TURBO : HAL_MODE_XR);

	for (i = 0; i < rt->rt_rate_count; i++) {
		AR5K_REG_WRITE(AR5K_AR5212_RATE_DUR(rt->rt_info[i].r_rate_code),
		    ath_hal_computetxtime(rt, 14,
		    rt->rt_info[i].r_control_rate, AH_FALSE));
	}

	if (!(channel->c_channel_flags & IEEE80211_CHAN_TURBO)) {
		rt = ar5k_ar5212_get_rate_table(HAL_MODE_11B);
		for (i = 0; i < rt->rt_rate_count; i++) {
			data = AR5K_AR5212_RATE_DUR(rt->rt_info[i].r_rate_code);
			AR5K_REG_WRITE(data,
			    ath_hal_computetxtime(rt, 14,
			    rt->rt_info[i].r_control_rate, AH_FALSE));
			if (rt->rt_info[i].r_short_preamble) {
				AR5K_REG_WRITE(data +
				    (rt->rt_info[i].r_short_preamble << 2),
				    ath_hal_computetxtime(rt, 14,
				    rt->rt_info[i].r_control_rate, AH_FALSE));
			}
		}
	}

	/*
	 * Set TX power (XXX use txpower from net80211)
	 */
	if (ar5k_ar5212_txpower(channel,
		AR5K_TUNE_DEFAULT_TXPOWER) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Write RF registers
	 */
	if (ar5k_rfregs(channel, mode) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Configure additional registers
	 */

	/* OFDM timings */
	if (channel->c_channel_flags & IEEE80211_CHAN_OFDM) {
		u_int32_t coef_scaled, coef_exp, coef_man, ds_coef_exp,
		    ds_coef_man, clock;

		clock = channel->c_channel_flags & IEEE80211_CHAN_T ? 80 : 40;
		coef_scaled = ((5 * (clock << 24)) / 2) / channel->c_channel;

		for (coef_exp = 31; coef_exp > 0; coef_exp--)
			if ((coef_scaled >> coef_exp) & 0x1)
				break;

		if (!coef_exp)
			return (AH_FALSE);

		coef_exp = 14 - (coef_exp - 24);
		coef_man = coef_scaled + (1 << (24 - coef_exp - 1));
		ds_coef_man = coef_man >> (24 - coef_exp);
		ds_coef_exp = coef_exp - 16;

		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_TIMING_3,
		    AR5K_AR5212_PHY_TIMING_3_DSC_MAN, ds_coef_man);
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_TIMING_3,
		    AR5K_AR5212_PHY_TIMING_3_DSC_EXP, ds_coef_exp);
	}

	if (ah_radio == AR5K_AR5111) {
		if (channel->c_channel_flags & IEEE80211_CHAN_B)
			AR5K_REG_ENABLE_BITS(AR5K_AR5212_TXCFG,
			    AR5K_AR5212_TXCFG_B_MODE);
		else
			AR5K_REG_DISABLE_BITS(AR5K_AR5212_TXCFG,
			    AR5K_AR5212_TXCFG_B_MODE);
	}

	/* Set antenna mode */
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x44),
	    ah_antenna[ee_mode][0], 0xfffffc06);

	ant[0] = HAL_ANT_FIXED_A;
	ant[1] = HAL_ANT_FIXED_B;

	if (ah_ant_diversity == AH_FALSE) {
		if (freq == AR5K_INI_RFGAIN_2GHZ)
			ant[0] = HAL_ANT_FIXED_B;
		else
			ant[1] = HAL_ANT_FIXED_A;
	}

	AR5K_REG_WRITE(AR5K_AR5212_PHY_ANT_SWITCH_TABLE_0,
	    ah_antenna[ee_mode][ant[0]]);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ANT_SWITCH_TABLE_1,
	    ah_antenna[ee_mode][ant[1]]);

	/* Commit values from EEPROM */
	if (ah_radio == AR5K_AR5111)
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_FC,
		    AR5K_AR5212_PHY_FC_TX_CLIP, ee->ee_tx_clip);

	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x5a),
	    AR5K_AR5212_PHY_NF_SVAL(ee->ee_noise_floor_thr[ee_mode]));

	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x11),
	    (ee->ee_switch_settling[ee_mode] << 7) & 0x3f80, 0xffffc07f);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x12),
	    (ee->ee_ant_tx_rx[ee_mode] << 12) & 0x3f000, 0xfffc0fff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x14),
	    (ee->ee_adc_desired_size[ee_mode] & 0x00ff) |
	    ((ee->ee_pga_desired_size[ee_mode] << 8) & 0xff00), 0xffff0000);

	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x0d),
	    (ee->ee_tx_end2xpa_disable[ee_mode] << 24) |
	    (ee->ee_tx_end2xpa_disable[ee_mode] << 16) |
	    (ee->ee_tx_frm2xpa_enable[ee_mode] << 8) |
	    (ee->ee_tx_frm2xpa_enable[ee_mode]));

	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x0a),
	    ee->ee_tx_end2xlna_enable[ee_mode] << 8, 0xffff00ff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x19),
	    (ee->ee_thr_62[ee_mode] << 12) & 0x7f000, 0xfff80fff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x49), 4, 0xffffff01);

	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
	    AR5K_AR5212_PHY_IQ_CORR_ENABLE |
	    (ee->ee_i_cal[ee_mode] << AR5K_AR5212_PHY_IQ_CORR_Q_I_COFF_S) |
	    ee->ee_q_cal[ee_mode]);

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_1) {
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_GAIN_2GHZ,
		    AR5K_AR5212_PHY_GAIN_2GHZ_MARGIN_TXRX,
		    ee->ee_margin_tx_rx[ee_mode]);
	}

	/*
	 * Restore saved values
	 */
	AR5K_REG_WRITE(AR5K_AR5212_DCU_SEQNUM(0), s_seq);
	AR5K_REG_WRITE(AR5K_AR5212_DEFAULT_ANTENNA, s_ant);
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PCICFG, s_led[0]);
	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR, s_led[1]);
	AR5K_REG_WRITE(AR5K_AR5212_GPIODO, s_led[2]);

	/*
	 * Misc
	 */
	bcopy(etherbroadcastaddr, mac, IEEE80211_ADDR_LEN);
	ar5k_ar5212_set_associd(mac, 0, 0);
	ar5k_ar5212_set_opmode();
	AR5K_REG_WRITE(AR5K_AR5212_PISR, 0xffffffff);
	AR5K_REG_WRITE(AR5K_AR5212_RSSI_THR, AR5K_TUNE_RSSI_THRES);

	/*
	 * Set Rx/Tx DMA Configuration
	 */
	/* not present in old HAL, ask Mick
	AR5K_REG_WRITE_BITS(AR5K_AR5212_TXCFG, AR5K_AR5212_TXCFG_SDMAMR,
	    AR5K_AR5212_DMASIZE_512B | AR5K_AR5212_TXCFG_DMASIZE);
	AR5K_REG_WRITE_BITS(AR5K_AR5212_RXCFG, AR5K_AR5212_RXCFG_SDMAMW,
	    AR5K_AR5212_DMASIZE_512B);
	*/

	/*
	 * Set channel and calibrate the PHY
	 */
	if (ar5k_channel(channel) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Enable the PHY and wait until completion
	 */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ACTIVE, AR5K_AR5212_PHY_ENABLE);

	data = AR5K_REG_READ(AR5K_AR5212_PHY_RX_DELAY) &
	    AR5K_AR5212_PHY_RX_DELAY_M;
	data = (channel->c_channel_flags & IEEE80211_CHAN_CCK) ?
	    ((data << 2) / 22) : (data / 10);

	AR5K_DELAY(100 + data);

	/*
	 * Start calibration
	 */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_AGCCTL,
	    AR5K_AR5212_PHY_AGCCTL_NF |
	    AR5K_AR5212_PHY_AGCCTL_CAL);

	if (channel->c_channel_flags & IEEE80211_CHAN_B) {
		ah_calibration = AH_FALSE;
	} else {
		ah_calibration = AH_TRUE;
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_IQ,
		    AR5K_AR5212_PHY_IQ_CAL_NUM_LOG_MAX, 15);
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
		    AR5K_AR5212_PHY_IQ_RUN);
	}

	/*
	 * Reset queues and start beacon timers at the end of the reset routine
	 */
	for (i = 0; i < ah_capabilities.cap_queues.q_tx_num; i++) {
		AR5K_REG_WRITE_Q(AR5K_AR5212_DCU_QCUMASK(i), i);
		if (ar5k_ar5212_reset_tx_queue(i) == AH_FALSE) {
			AR5K_PRINTF("failed to reset TX queue #%d\n", i);
			return (AH_FALSE);
		}
	}

	/* Pre-enable interrupts */
	ar5k_ar5212_set_intr(HAL_INT_RX | HAL_INT_TX | HAL_INT_FATAL);

	/*
	 * Set RF kill flags if supported by the device (read from the EEPROM)
	 */
	if (AR5K_EEPROM_HDR_RFKILL(ah_capabilities.cap_eeprom.ee_header)) {
		ar5k_ar5212_set_gpio_input(0);
		if ((ah_gpio[0] = ar5k_ar5212_get_gpio(0)) == 0)
			ar5k_ar5212_set_gpio_intr(0, 1);
		else
			ar5k_ar5212_set_gpio_intr(0, 0);
	}

	/*
	 * Set the 32MHz reference clock
	 */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCR, AR5K_AR5212_PHY_SCR_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SLMT, AR5K_AR5212_PHY_SLMT_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCAL, AR5K_AR5212_PHY_SCAL_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCLOCK, AR5K_AR5212_PHY_SCLOCK_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SDELAY, AR5K_AR5212_PHY_SDELAY_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SPENDING, ah_radio == AR5K_AR5111 ?
	    AR5K_AR5212_PHY_SPENDING_AR5111 : AR5K_AR5212_PHY_SPENDING_AR5112);

	/* 
	 * Disable beacons and reset the register
	 */
	AR5K_REG_DISABLE_BITS(AR5K_AR5212_BEACON,
	    AR5K_AR5212_BEACON_ENABLE | AR5K_AR5212_BEACON_RESET_TSF);

	return (AH_TRUE);
}

void
OpenHAL5212::ar5k_ar5212_set_opmode()
{
	u_int32_t pcu_reg, low_id, high_id;

	pcu_reg = 0;

	switch (ah_op_mode) {
	case IEEE80211_M_IBSS:
		pcu_reg |= AR5K_AR5212_STA_ID1_ADHOC |
		    AR5K_AR5212_STA_ID1_DESC_ANTENNA;
		break;

	case IEEE80211_M_HOSTAP:
		pcu_reg |= AR5K_AR5212_STA_ID1_AP |
		    AR5K_AR5212_STA_ID1_RTS_DEFAULT_ANTENNA;
		break;

	case IEEE80211_M_STA:
	case IEEE80211_M_MONITOR:
		pcu_reg |= AR5K_AR5212_STA_ID1_DEFAULT_ANTENNA;
		break;

	default:
		return;
	}

	/*
	 * Set PCU registers
	 */
	bcopy(&(ah_sta_id[0]), &low_id, 4);
	bcopy(&(ah_sta_id[4]), &high_id, 2);
	AR5K_REG_WRITE(AR5K_AR5212_STA_ID0, low_id);
	AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, pcu_reg | high_id);

	return;
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_calibrate(HAL_CHANNEL *channel)
{
	u_int32_t i_pwr, q_pwr;
	int32_t iq_corr, i_coff, i_coffd, q_coff, q_coffd;

	if (ah_calibration == AH_FALSE ||
	    AR5K_REG_READ(AR5K_AR5212_PHY_IQ) & AR5K_AR5212_PHY_IQ_RUN)
		goto done;

	ah_calibration = AH_FALSE;

	iq_corr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_CORR);
	i_pwr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_PWR_I);
	q_pwr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_PWR_Q);
	i_coffd = ((i_pwr >> 1) + (q_pwr >> 1)) >> 7;
	q_coffd = q_pwr >> 6;

	if (i_coffd == 0 || q_coffd == 0)
		goto done;

	i_coff = ((-iq_corr) / i_coffd) & 0x3f;
	q_coff = (((int32_t)i_pwr / q_coffd) - 64) & 0x1f;

	/* Commit new IQ value */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
	    AR5K_AR5212_PHY_IQ_CORR_ENABLE |
	    ((u_int32_t)q_coff) |
	    ((u_int32_t)i_coff << AR5K_AR5212_PHY_IQ_CORR_Q_I_COFF_S));

 done:
	/* Start noise floor calibration */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_AGCCTL,
	    AR5K_AR5212_PHY_AGCCTL_NF);

	/* Request RF gain */
	if (channel->c_channel_flags & IEEE80211_CHAN_5GHZ) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_PAPD_PROBE,
		    AR5K_REG_SM(ah_txpower.txp_max,
		    AR5K_AR5212_PHY_PAPD_PROBE_TXPOWER) |
		    AR5K_AR5212_PHY_PAPD_PROBE_TX_NEXT);
		ah_rf_gain = HAL_RFGAIN_READ_REQUESTED;
	}

	return (AH_TRUE);
}

#pragma mark -

/*
 * Transmit functions
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_update_tx_triglevel(HAL_BOOL increase)
{
	u_int32_t trigger_level, imr;
	HAL_BOOL status = AH_FALSE;

	/*
	 * Disable interrupts by setting the mask
	 */
	imr = ar5k_ar5212_set_intr(ah_imr & ~HAL_INT_GLOBAL);

	trigger_level = AR5K_REG_MS(AR5K_REG_READ(AR5K_AR5212_TXCFG),
	    AR5K_AR5212_TXCFG_TXFULL);

	if (increase == AH_FALSE) {
		if (--trigger_level < AR5K_TUNE_MIN_TX_FIFO_THRES)
			goto done;
	} else
		trigger_level +=
		    ((AR5K_TUNE_MAX_TX_FIFO_THRES - trigger_level) / 2);

	/*
	 * Update trigger level on success
	 */
	AR5K_REG_WRITE_BITS(AR5K_AR5212_TXCFG,
	    AR5K_AR5212_TXCFG_TXFULL, trigger_level);
	status = AH_TRUE;

 done:
	/*
	 * Restore interrupt mask
	 */
	ar5k_ar5212_set_intr(imr);

	return (status);
}

int
OpenHAL5212::ar5k_ar5212_setup_tx_queue(HAL_TX_QUEUE queue_type,
    const HAL_TXQ_INFO *queue_info)
{
	u_int queue;

	/*
	 * Get queue by type
	 */
	if (queue_type == HAL_TX_QUEUE_DATA) {
		for (queue = HAL_TX_QUEUE_ID_DATA_MIN;
		     ah_txq[queue].tqi_type != HAL_TX_QUEUE_INACTIVE;
		     queue++)
			if (queue > HAL_TX_QUEUE_ID_DATA_MAX)
				return (-1);
	} else if (queue_type == HAL_TX_QUEUE_PSPOLL) {
		queue = HAL_TX_QUEUE_ID_PSPOLL;
	} else if (queue_type == HAL_TX_QUEUE_BEACON) {
		queue = HAL_TX_QUEUE_ID_BEACON;
	} else if (queue_type == HAL_TX_QUEUE_CAB) {
		queue = HAL_TX_QUEUE_ID_CAB;
	} else
		return (-1);

	/*
	 * Setup internal queue structure
	 */
	bzero(&ah_txq[queue], sizeof(HAL_TXQ_INFO));
	ah_txq[queue].tqi_type = queue_type;

	if (queue_info != NULL) {
		if (ar5k_ar5212_setup_tx_queueprops(queue, queue_info)
		    != AH_TRUE)
			return (-1);
	}

	AR5K_Q_ENABLE_BITS(ah_txq_interrupts, queue);

	return (queue);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_setup_tx_queueprops(int queue,
    const HAL_TXQ_INFO *queue_info)
{
	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	if (ah_txq[queue].tqi_type == HAL_TX_QUEUE_INACTIVE)
		return (AH_FALSE);

	bcopy(queue_info, &ah_txq[queue], sizeof(HAL_TXQ_INFO));

	if (queue_info->tqi_type == HAL_TX_QUEUE_DATA &&
	    (queue_info->tqi_subtype >= HAL_WME_AC_VI) &&
	    (queue_info->tqi_subtype <= HAL_WME_UPSD))
		ah_txq[queue].tqi_flags |=
		    AR5K_TXQ_FLAG_POST_FR_BKOFF_DIS;

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_release_tx_queue(u_int queue)
{
	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	/* This queue will be skipped in further operations */
	ah_txq[queue].tqi_type = HAL_TX_QUEUE_INACTIVE;
	AR5K_Q_DISABLE_BITS(ah_txq_interrupts, queue);

	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_reset_tx_queue(u_int queue)
{
	u_int32_t cw_min, cw_max, retry_lg, retry_sh;
	struct ieee80211_channel *channel = (struct ieee80211_channel*)
	    &ah_current_channel;
	HAL_TXQ_INFO *tq;

	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	tq = &ah_txq[queue];

	if (tq->tqi_type == HAL_TX_QUEUE_INACTIVE)
		return (AH_TRUE);

	/*
	 * Set registers by channel mode
	 */
	if (IEEE80211_IS_CHAN_XR(channel)) {
		ah_cw_min = AR5K_TUNE_CWMIN_XR;
		cw_max = ah_cw_max = AR5K_TUNE_CWMAX_XR;
		ah_aifs = AR5K_TUNE_AIFS_XR;
	} else if (IEEE80211_IS_CHAN_B(channel)) {
		ah_cw_min = AR5K_TUNE_CWMIN_11B;
		cw_max = ah_cw_max = AR5K_TUNE_CWMAX_11B;
		ah_aifs = AR5K_TUNE_AIFS_11B;
	} else {
		ah_cw_min = AR5K_TUNE_CWMIN;
		ah_cw_max = AR5K_TUNE_CWMAX;
		ah_aifs = AR5K_TUNE_AIFS;
	}

	/*
	 * Set retry limits
	 */
	if (ah_software_retry == AH_TRUE) {
		/* XXX Need to test this */
		retry_lg = ah_limit_tx_retries;
		retry_sh = retry_lg =
		    retry_lg > AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY ?
		    AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY : retry_lg;
	} else {
		retry_lg = AR5K_INIT_LG_RETRY;
		retry_sh = AR5K_INIT_SH_RETRY;
	}

	AR5K_REG_WRITE(AR5K_AR5212_DCU_RETRY_LMT(queue),
	    AR5K_REG_SM(AR5K_INIT_SLG_RETRY,
	    AR5K_AR5212_DCU_RETRY_LMT_SLG_RETRY) |
	    AR5K_REG_SM(AR5K_INIT_SSH_RETRY,
	    AR5K_AR5212_DCU_RETRY_LMT_SSH_RETRY) |
	    AR5K_REG_SM(retry_lg, AR5K_AR5212_DCU_RETRY_LMT_LG_RETRY) |
	    AR5K_REG_SM(retry_sh, AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY));

	/*
	 * Set initial content window (cw_min/cw_max)
	 */
	cw_min = 1;
	while (cw_min < ah_cw_min)
		cw_min = (cw_min << 1) | 1;

	cw_min = tq->tqi_cw_min < 0 ?
	    (cw_min >> (-tq->tqi_cw_min)) :
	    ((cw_min << tq->tqi_cw_min) + (1 << tq->tqi_cw_min) - 1);
	cw_max = tq->tqi_cw_max < 0 ?
	    (cw_max >> (-tq->tqi_cw_max)) :
	    ((cw_max << tq->tqi_cw_max) + (1 << tq->tqi_cw_max) - 1);

	AR5K_REG_WRITE(AR5K_AR5212_DCU_LCL_IFS(queue),
	    AR5K_REG_SM(cw_min, AR5K_AR5212_DCU_LCL_IFS_CW_MIN) |
	    AR5K_REG_SM(cw_max, AR5K_AR5212_DCU_LCL_IFS_CW_MAX) |
	    AR5K_REG_SM(ah_aifs + tq->tqi_aifs,
	    AR5K_AR5212_DCU_LCL_IFS_AIFS));

	/*
	 * Set misc registers
	 */
	AR5K_REG_WRITE(AR5K_AR5212_QCU_MISC(queue),
	    AR5K_AR5212_QCU_MISC_DCU_EARLY);

	if (tq->tqi_cbr_period) {
		AR5K_REG_WRITE(AR5K_AR5212_QCU_CBRCFG(queue),
		    AR5K_REG_SM(tq->tqi_cbr_period,
		    AR5K_AR5212_QCU_CBRCFG_INTVAL) |
		    AR5K_REG_SM(tq->tqi_cbr_overflow_limit,
		    AR5K_AR5212_QCU_CBRCFG_ORN_THRES));
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_CBR);
		if (tq->tqi_cbr_overflow_limit)
			AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
			    AR5K_AR5212_QCU_MISC_CBR_THRES_ENABLE);
	}

	if (tq->tqi_ready_time) {
		AR5K_REG_WRITE(AR5K_AR5212_QCU_RDYTIMECFG(queue),
		    AR5K_REG_SM(tq->tqi_ready_time,
		    AR5K_AR5212_QCU_RDYTIMECFG_INTVAL) |
		    AR5K_AR5212_QCU_RDYTIMECFG_ENABLE);
	}

	if (tq->tqi_burst_time) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_CHAN_TIME(queue),
		    AR5K_REG_SM(tq->tqi_burst_time,
		    AR5K_AR5212_DCU_CHAN_TIME_DUR) |
		    AR5K_AR5212_DCU_CHAN_TIME_ENABLE);

		if (tq->tqi_flags & AR5K_TXQ_FLAG_RDYTIME_EXP_POLICY_ENABLE) {
			AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
			    AR5K_AR5212_QCU_MISC_TXE);
		}
	}

	if (tq->tqi_flags & AR5K_TXQ_FLAG_BACKOFF_DISABLE) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_MISC(queue),
		    AR5K_AR5212_DCU_MISC_POST_FR_BKOFF_DIS);
	}

	if (tq->tqi_flags & AR5K_TXQ_FLAG_FRAG_BURST_BACKOFF_ENABLE) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_MISC(queue),
		    AR5K_AR5212_DCU_MISC_BACKOFF_FRAG);
	}

	/*
	 * Set registers by queue type
	 */
	switch (tq->tqi_type) {
	case HAL_TX_QUEUE_BEACON:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_DBA_GT |
		    AR5K_AR5212_QCU_MISC_CBREXP_BCN |
		    AR5K_AR5212_QCU_MISC_BCN_ENABLE);

		AR5K_REG_ENABLE_BITS(AR5K_AR5212_DCU_MISC(queue),
		    (AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL <<
		    AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL) |
		    AR5K_AR5212_DCU_MISC_POST_FR_BKOFF_DIS |
		    AR5K_AR5212_DCU_MISC_BCN_ENABLE);

		AR5K_REG_WRITE(AR5K_AR5212_QCU_RDYTIMECFG(queue),
		    ((AR5K_TUNE_BEACON_INTERVAL -
		    (AR5K_TUNE_SW_BEACON_RESP - AR5K_TUNE_DMA_BEACON_RESP) -
		    AR5K_TUNE_ADDITIONAL_SWBA_BACKOFF) * 1024) |
		    AR5K_AR5212_QCU_RDYTIMECFG_ENABLE);
		break;

	case HAL_TX_QUEUE_CAB:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_DBA_GT |
		    AR5K_AR5212_QCU_MISC_CBREXP |
		    AR5K_AR5212_QCU_MISC_CBREXP_BCN);

		AR5K_REG_ENABLE_BITS(AR5K_AR5212_DCU_MISC(queue),
		    (AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL <<
		    AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL));
		break;

	case HAL_TX_QUEUE_PSPOLL:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_CBREXP);
		break;

	case HAL_TX_QUEUE_DATA:
	default:
		break;
	}

	/*
	 * Enable tx queue in the secondary interrupt mask registers
	 */
	AR5K_REG_WRITE(AR5K_AR5212_SIMR0,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR0_QCU_TXOK) |
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR0_QCU_TXDESC));
	AR5K_REG_WRITE(AR5K_AR5212_SIMR1,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR1_QCU_TXERR));
	AR5K_REG_WRITE(AR5K_AR5212_SIMR2,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR2_QCU_TXURN));

	return (AH_TRUE);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_tx_buf(u_int queue)
{
	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	/*
	 * Get the transmit queue descriptor pointer from the selected queue
	 */
	return (AR5K_REG_READ(AR5K_AR5212_QCU_TXDP(queue)));
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_put_tx_buf(u_int queue, u_int32_t phys_addr)
{
	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	/*
	 * Set the transmit queue descriptor pointer for the selected queue
	 * (this won't work if the queue is still active)
	 */
	if (AR5K_REG_READ_Q(AR5K_AR5212_QCU_TXE, queue))
		return (AH_FALSE);

	AR5K_REG_WRITE(AR5K_AR5212_QCU_TXDP(queue), phys_addr);

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_tx_start(u_int queue)
{
	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	/* Return if queue is disabled */
	if (AR5K_REG_READ_Q(AR5K_AR5212_QCU_TXD, queue))
		return (AH_FALSE);

	/* Start queue */
	AR5K_REG_WRITE_Q(AR5K_AR5212_QCU_TXE, queue);

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_stop_tx_dma(u_int queue)
{
	int i = 100, pending;

	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	/*
	 * Schedule TX disable and wait until queue is empty
	 */
	AR5K_REG_WRITE_Q(AR5K_AR5212_QCU_TXD, queue);

	do {
		pending = AR5K_REG_READ(AR5K_AR5212_QCU_STS(queue)) &
		     AR5K_AR5212_QCU_STS_FRMPENDCNT;
		AR5K_DELAY(100);
	} while (--i && pending);

	/* Clear register */
	AR5K_REG_WRITE(AR5K_AR5212_QCU_TXD, 0);

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_setup_tx_desc(struct ath_desc *desc,
    u_int packet_length, u_int header_length, HAL_PKT_TYPE type, u_int tx_power,
    u_int tx_rate0, u_int tx_tries0, u_int key_index, u_int antenna_mode,
    u_int flags, u_int rtscts_rate, u_int rtscts_duration)
{
	struct ar5k_ar5212_tx_desc *tx_desc;

	tx_desc = (struct ar5k_ar5212_tx_desc*)&desc->ds_ctl0;

	/*
	 * Validate input
	 */
	if (tx_tries0 == 0)
		return (AH_FALSE);

	if ((tx_desc->tx_control_0 = (packet_length &
	    AR5K_AR5212_DESC_TX_CTL0_FRAME_LEN)) != packet_length)
		return (AH_FALSE);

	tx_desc->tx_control_0 |=
	    AR5K_REG_SM(tx_power, AR5K_AR5212_DESC_TX_CTL0_XMIT_POWER) |
	    AR5K_REG_SM(antenna_mode, AR5K_AR5212_DESC_TX_CTL0_ANT_MODE_XMIT);
	tx_desc->tx_control_1 =
	    AR5K_REG_SM(type, AR5K_AR5212_DESC_TX_CTL1_FRAME_TYPE);
	tx_desc->tx_control_2 =
	    AR5K_REG_SM(tx_tries0 + AR5K_TUNE_HWTXTRIES,
	    AR5K_AR5212_DESC_TX_CTL2_XMIT_TRIES0);
	tx_desc->tx_control_3 =
	    tx_rate0 & AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE0;

#define _TX_FLAGS(_c, _flag)						\
	if (flags & HAL_TXDESC_##_flag)					\
		tx_desc->tx_control_##_c |=				\
			AR5K_AR5212_DESC_TX_CTL##_c##_##_flag

	_TX_FLAGS(0, CLRDMASK);
	_TX_FLAGS(0, VEOL);
	_TX_FLAGS(0, INTREQ);
	_TX_FLAGS(0, RTSENA);
	_TX_FLAGS(0, CTSENA);
	_TX_FLAGS(1, NOACK);

#undef _TX_FLAGS

	/*
	 * WEP crap
	 */
	if (key_index != HAL_TXKEYIX_INVALID) {
		tx_desc->tx_control_0 |=
		    AR5K_AR5212_DESC_TX_CTL0_ENCRYPT_KEY_VALID;
		tx_desc->tx_control_1 |=
		    AR5K_REG_SM(key_index,
		    AR5K_AR5212_DESC_TX_CTL1_ENCRYPT_KEY_INDEX);
	}

	/*
	 * RTS/CTS
	 */
	if (flags & (HAL_TXDESC_RTSENA | HAL_TXDESC_CTSENA)) {
		if ((flags & HAL_TXDESC_RTSENA) &&
		    (flags & HAL_TXDESC_CTSENA))
			return (AH_FALSE);
		tx_desc->tx_control_2 |=
		    rtscts_duration & AR5K_AR5212_DESC_TX_CTL2_RTS_DURATION;
		tx_desc->tx_control_3 |=
		    AR5K_REG_SM(rtscts_rate,
		    AR5K_AR5212_DESC_TX_CTL3_RTS_CTS_RATE);
	}

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_fill_tx_desc(struct ath_desc *desc,
    u_int segment_length, HAL_BOOL first_segment, HAL_BOOL last_segment)
{
	struct ar5k_ar5212_tx_desc *tx_desc;

	tx_desc = (struct ar5k_ar5212_tx_desc*)&desc->ds_ctl0;

	/* Clear status descriptor */
	bzero((void *)desc->ds_hw, sizeof(desc->ds_hw));

	/* Validate segment length and initialize the descriptor */
	if ((tx_desc->tx_control_1 = (segment_length &
	    AR5K_AR5212_DESC_TX_CTL1_BUF_LEN)) != segment_length)
		return (AH_FALSE);

	if (first_segment != AH_TRUE)
		tx_desc->tx_control_0 &= ~AR5K_AR5212_DESC_TX_CTL0_FRAME_LEN;

	if (last_segment != AH_TRUE)
		tx_desc->tx_control_1 |= AR5K_AR5212_DESC_TX_CTL1_MORE;

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_setup_xtx_desc(struct ath_desc *desc,
    u_int tx_rate1, u_int tx_tries1, u_int tx_rate2, u_int tx_tries2,
    u_int tx_rate3, u_int tx_tries3)
{
	struct ar5k_ar5212_tx_desc *tx_desc;

	tx_desc = (struct ar5k_ar5212_tx_desc*)&desc->ds_ctl0;

#define _XTX_TRIES(_n)							\
	if (tx_tries##_n) {						\
		tx_desc->tx_control_2 |=				\
		    AR5K_REG_SM(tx_tries##_n,				\
		    AR5K_AR5212_DESC_TX_CTL2_XMIT_TRIES##_n);		\
		tx_desc->tx_control_3 |=				\
		    AR5K_REG_SM(tx_rate##_n,				\
		    AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE##_n);		\
	}

	_XTX_TRIES(1);
	_XTX_TRIES(2);
	_XTX_TRIES(3);

#undef _XTX_TRIES

	return (AH_TRUE);
}

HAL_STATUS
OpenHAL5212::ar5k_ar5212_proc_tx_desc(struct ath_desc *desc)
{
	struct ar5k_ar5212_tx_status *tx_status;
	struct ar5k_ar5212_tx_desc *tx_desc;

	tx_desc = (struct ar5k_ar5212_tx_desc*)&desc->ds_ctl0;
	tx_status = (struct ar5k_ar5212_tx_status*)&desc->ds_hw[2];

	/* No frame has been send or error */
	if ((tx_status->tx_status_1 & AR5K_AR5212_DESC_TX_STATUS1_DONE) == 0)
		return (HAL_EINPROGRESS);

	/*
	 * Get descriptor status
	 */
	desc->ds_us.tx.ts_tstamp =
	    AR5K_REG_MS(tx_status->tx_status_0,
	    AR5K_AR5212_DESC_TX_STATUS0_SEND_TIMESTAMP);
	desc->ds_us.tx.ts_shortretry =
	    AR5K_REG_MS(tx_status->tx_status_0,
	    AR5K_AR5212_DESC_TX_STATUS0_RTS_FAIL_COUNT);
	desc->ds_us.tx.ts_longretry =
	    AR5K_REG_MS(tx_status->tx_status_0,
	    AR5K_AR5212_DESC_TX_STATUS0_DATA_FAIL_COUNT);
	desc->ds_us.tx.ts_seqnum =
	    AR5K_REG_MS(tx_status->tx_status_1,
	    AR5K_AR5212_DESC_TX_STATUS1_SEQ_NUM);
	desc->ds_us.tx.ts_rssi =
	    AR5K_REG_MS(tx_status->tx_status_1,
	    AR5K_AR5212_DESC_TX_STATUS1_ACK_SIG_STRENGTH);
	desc->ds_us.tx.ts_antenna = (tx_status->tx_status_1 &
	    AR5K_AR5212_DESC_TX_STATUS1_XMIT_ANTENNA) ? 2 : 1;
	desc->ds_us.tx.ts_status = 0;

	switch (AR5K_REG_MS(tx_status->tx_status_1,
	    AR5K_AR5212_DESC_TX_STATUS1_FINAL_TS_INDEX)) {
	case 0:
		desc->ds_us.tx.ts_rate = tx_desc->tx_control_3 &
		    AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE0;
		break;
	case 1:
		desc->ds_us.tx.ts_rate =
		    AR5K_REG_MS(tx_desc->tx_control_3,
		    AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE1);
		desc->ds_us.tx.ts_longretry +=
		    AR5K_REG_MS(tx_desc->tx_control_2,
		    AR5K_AR5212_DESC_TX_CTL2_XMIT_TRIES1);
		break;
	case 2:
		desc->ds_us.tx.ts_rate =
		    AR5K_REG_MS(tx_desc->tx_control_3,
		    AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE2);
		desc->ds_us.tx.ts_longretry +=
		    AR5K_REG_MS(tx_desc->tx_control_2,
		    AR5K_AR5212_DESC_TX_CTL2_XMIT_TRIES2);
		break;
	case 3:
		desc->ds_us.tx.ts_rate =
		    AR5K_REG_MS(tx_desc->tx_control_3,
		    AR5K_AR5212_DESC_TX_CTL3_XMIT_RATE3);
		desc->ds_us.tx.ts_longretry +=
		    AR5K_REG_MS(tx_desc->tx_control_2,
		    AR5K_AR5212_DESC_TX_CTL2_XMIT_TRIES3);
		break;
	}

	if ((tx_status->tx_status_0 &
	    AR5K_AR5212_DESC_TX_STATUS0_FRAME_XMIT_OK) == 0) {
		if (tx_status->tx_status_0 &
		    AR5K_AR5212_DESC_TX_STATUS0_EXCESSIVE_RETRIES)
			desc->ds_us.tx.ts_status |= HAL_TXERR_XRETRY;

		if (tx_status->tx_status_0 &
		    AR5K_AR5212_DESC_TX_STATUS0_FIFO_UNDERRUN)
			desc->ds_us.tx.ts_status |= HAL_TXERR_FIFO;

		if (tx_status->tx_status_0 &
		    AR5K_AR5212_DESC_TX_STATUS0_FILTERED)
			desc->ds_us.tx.ts_status |= HAL_TXERR_FILT;
	}

	return (HAL_OK);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_has_veol()
{
	return (AH_TRUE);
}

#pragma mark -

/*
 * Receive functions
 */

u_int32_t
OpenHAL5212::ar5k_ar5212_get_rx_buf()
{
	return (AR5K_REG_READ(AR5K_AR5212_RXDP));
}

void
OpenHAL5212::ar5k_ar5212_put_rx_buf(u_int32_t phys_addr)
{
	AR5K_REG_WRITE(AR5K_AR5212_RXDP, phys_addr);
}

void
OpenHAL5212::ar5k_ar5212_start_rx()
{
	AR5K_REG_WRITE(AR5K_AR5212_CR, AR5K_AR5212_CR_RXE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_stop_rx_dma()
{
	int i;

	AR5K_REG_WRITE(AR5K_AR5212_CR, AR5K_AR5212_CR_RXD);

	/*
	 * It may take some time to disable the DMA receive unit
	 */
	for (i = 2000;
	     i > 0 && (AR5K_REG_READ(AR5K_AR5212_CR) & AR5K_AR5212_CR_RXE) != 0;
	     i--)
		AR5K_DELAY(10);

	return (i > 0 ? AH_TRUE : AH_FALSE);
}

void
OpenHAL5212::ar5k_ar5212_start_rx_pcu()
{
	AR5K_REG_DISABLE_BITS(AR5K_AR5212_DIAG_SW, AR5K_AR5212_DIAG_SW_DIS_RX);
}

void
OpenHAL5212::ar5k_ar5212_stop_pcu_recv()
{
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_DIAG_SW, AR5K_AR5212_DIAG_SW_DIS_RX);
}

void
OpenHAL5212::ar5k_ar5212_set_mcast_filter(u_int32_t filter0,
    u_int32_t filter1)
{
	/* Set the multicat filter */
	AR5K_REG_WRITE(AR5K_AR5212_MCAST_FIL0, filter0);
	AR5K_REG_WRITE(AR5K_AR5212_MCAST_FIL1, filter1);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_mcast_filterindex(u_int32_t index)
{
	if (index >= 64) {
	    return (AH_FALSE);
	} else if (index >= 32) {
	    AR5K_REG_ENABLE_BITS(AR5K_AR5212_MCAST_FIL1,
		(1 << (index - 32)));
	} else {
	    AR5K_REG_ENABLE_BITS(AR5K_AR5212_MCAST_FIL0,
		(1 << index));
	}

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_clear_mcast_filter_idx(u_int32_t index)
{

	if (index >= 64) {
	    return (AH_FALSE);
	} else if (index >= 32) {
	    AR5K_REG_DISABLE_BITS(AR5K_AR5212_MCAST_FIL1,
		(1 << (index - 32)));
	} else {
	    AR5K_REG_DISABLE_BITS(AR5K_AR5212_MCAST_FIL0,
		(1 << index));
	}

	return (AH_TRUE);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_rx_filter()
{
	u_int32_t data, filter = 0;

	filter = AR5K_REG_READ(AR5K_AR5212_RX_FILTER);
	data = AR5K_REG_READ(AR5K_AR5212_PHY_ERR_FIL);

	if (data & AR5K_AR5212_PHY_ERR_FIL_RADAR)
		filter |= HAL_RX_FILTER_PHYRADAR;
	if (data & (AR5K_AR5212_PHY_ERR_FIL_OFDM |
	    AR5K_AR5212_PHY_ERR_FIL_CCK))
		filter |= HAL_RX_FILTER_PHYERR;

	return (filter);
}

void
OpenHAL5212::ar5k_ar5212_set_rx_filter(u_int32_t filter)
{
	u_int32_t data = 0;

	if (filter & HAL_RX_FILTER_PHYRADAR)
		data |= AR5K_AR5212_PHY_ERR_FIL_RADAR;
	if (filter & HAL_RX_FILTER_PHYERR)
		data |= AR5K_AR5212_PHY_ERR_FIL_OFDM |
		    AR5K_AR5212_PHY_ERR_FIL_CCK;

	if (data) {
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_RXCFG,
		    AR5K_AR5212_RXCFG_ZLFDMA);
	} else {
		AR5K_REG_DISABLE_BITS(AR5K_AR5212_RXCFG,
		    AR5K_AR5212_RXCFG_ZLFDMA);
	}

	AR5K_REG_WRITE(AR5K_AR5212_RX_FILTER, filter & 0xff);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ERR_FIL, data);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_setup_rx_desc(struct ath_desc *desc,
    u_int32_t size, u_int flags)
{
	struct ar5k_ar5212_rx_desc *rx_desc;

	rx_desc = (struct ar5k_ar5212_rx_desc*)&desc->ds_ctl0;

	if ((rx_desc->rx_control_1 = (size &
	    AR5K_AR5212_DESC_RX_CTL1_BUF_LEN)) != size)
		return (AH_FALSE);

	if (flags & HAL_RXDESC_INTREQ)
		rx_desc->rx_control_1 |= AR5K_AR5212_DESC_RX_CTL1_INTREQ;

	return (AH_TRUE);
}

HAL_STATUS
OpenHAL5212::ar5k_ar5212_proc_rx_desc(struct ath_desc *desc,
    u_int32_t phys_addr, struct ath_desc *next)
{
	struct ar5k_ar5212_rx_status *rx_status;
	struct ar5k_ar5212_rx_error *rx_err;

	rx_status = (struct ar5k_ar5212_rx_status*)&desc->ds_hw[0];

	/* Overlay on error */
	rx_err = (struct ar5k_ar5212_rx_error*)&desc->ds_hw[0];

	/* No frame received / not ready */
	if ((rx_status->rx_status_1 & AR5K_AR5212_DESC_RX_STATUS1_DONE) == 0)
		return (HAL_EINPROGRESS);

	/*
	 * Frame receive status
	 */
	desc->ds_us.rx.rs_datalen = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_DATA_LEN;
	desc->ds_us.rx.rs_rssi =
	    AR5K_REG_MS(rx_status->rx_status_0,
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_SIGNAL);
	desc->ds_us.rx.rs_rate =
	    AR5K_REG_MS(rx_status->rx_status_0,
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_RATE);
	desc->ds_us.rx.rs_antenna = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_ANTENNA;
	desc->ds_us.rx.rs_more = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_MORE;
	desc->ds_us.rx.rs_tstamp =
	    AR5K_REG_MS(rx_status->rx_status_1,
	    AR5K_AR5212_DESC_RX_STATUS1_RECEIVE_TIMESTAMP);
	desc->ds_us.rx.rs_status = 0;

	/*
	 * Key table status
	 */
	if (rx_status->rx_status_1 &
	    AR5K_AR5212_DESC_RX_STATUS1_KEY_INDEX_VALID) {
		desc->ds_us.rx.rs_keyix =
		    AR5K_REG_MS(rx_status->rx_status_1,
		    AR5K_AR5212_DESC_RX_STATUS1_KEY_INDEX);
	} else {
		desc->ds_us.rx.rs_keyix = HAL_RXKEYIX_INVALID;
	}

	/*
	 * Receive/descriptor errors
	 */
	if ((rx_status->rx_status_1 &
	    AR5K_AR5212_DESC_RX_STATUS1_FRAME_RECEIVE_OK) == 0) {
		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_CRC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_CRC;

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_PHY_ERROR) {
			desc->ds_us.rx.rs_status |= HAL_RXERR_PHY;
			desc->ds_us.rx.rs_phyerr =
			    AR5K_REG_MS(rx_err->rx_error_1,
			    AR5K_AR5212_DESC_RX_ERROR1_PHY_ERROR_CODE);
		}

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_DECRYPT_CRC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_DECRYPT;

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_MIC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_MIC;
	}

	return (HAL_OK);
}

void
OpenHAL5212::ar5k_ar5212_set_rx_signal()
{
	/* Signal state monitoring is not yet supported */
	
	/* went away in OpenBSD HAL, ask Reyk */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_RX_FILTER,
		AR5K_AR5212_RX_FILTER_PROMISC);
}

#pragma mark -

/*
 * Misc functions
 */

void
OpenHAL5212::ar5k_ar5212_dump_state()
{
#ifdef AR5K_DEBUG
#define AR5K_PRINT_REGISTER(_x)									\
	IOLog("(%s: %08x)\n", #_x, AR5K_REG_READ(AR5K_AR5212_##_x));	\
	IOSleep(100);

	IOLog("MAC registers:\n");
	AR5K_PRINT_REGISTER(CR);
	AR5K_PRINT_REGISTER(CFG);
	AR5K_PRINT_REGISTER(IER);
	AR5K_PRINT_REGISTER(TXCFG);
	AR5K_PRINT_REGISTER(RXCFG);
	AR5K_PRINT_REGISTER(MIBC);
	AR5K_PRINT_REGISTER(TOPS);
	AR5K_PRINT_REGISTER(RXNOFRM);
	AR5K_PRINT_REGISTER(RPGTO);
	AR5K_PRINT_REGISTER(RFCNT);
	AR5K_PRINT_REGISTER(MISC);
	AR5K_PRINT_REGISTER(PISR);
	AR5K_PRINT_REGISTER(SISR0);
	AR5K_PRINT_REGISTER(SISR1);
	AR5K_PRINT_REGISTER(SISR3);
	AR5K_PRINT_REGISTER(SISR4);
	AR5K_PRINT_REGISTER(DCM_ADDR);
	AR5K_PRINT_REGISTER(DCM_DATA);
	AR5K_PRINT_REGISTER(DCCFG);
	AR5K_PRINT_REGISTER(CCFG);
	AR5K_PRINT_REGISTER(CCFG_CUP);
	AR5K_PRINT_REGISTER(CPC0);
	AR5K_PRINT_REGISTER(CPC1);
	AR5K_PRINT_REGISTER(CPC2);
	AR5K_PRINT_REGISTER(CPCORN);
	AR5K_PRINT_REGISTER(QCU_TXE);
	AR5K_PRINT_REGISTER(QCU_TXD);
	AR5K_PRINT_REGISTER(DCU_GBL_IFS_SIFS);
	AR5K_PRINT_REGISTER(DCU_GBL_IFS_SLOT);
	AR5K_PRINT_REGISTER(DCU_FP);
	AR5K_PRINT_REGISTER(DCU_TXP);
	AR5K_PRINT_REGISTER(DCU_TX_FILTER);
	AR5K_PRINT_REGISTER(RC);
	AR5K_PRINT_REGISTER(SCR);
	AR5K_PRINT_REGISTER(INTPEND);
	AR5K_PRINT_REGISTER(PCICFG);
	AR5K_PRINT_REGISTER(GPIOCR);
	AR5K_PRINT_REGISTER(GPIODO);
	AR5K_PRINT_REGISTER(SREV);
	AR5K_PRINT_REGISTER(EEPROM_BASE);
	AR5K_PRINT_REGISTER(EEPROM_DATA);
	AR5K_PRINT_REGISTER(EEPROM_CMD);
	AR5K_PRINT_REGISTER(EEPROM_CFG);
	AR5K_PRINT_REGISTER(PCU_MIN);
	AR5K_PRINT_REGISTER(STA_ID0);
	AR5K_PRINT_REGISTER(STA_ID1);
	AR5K_PRINT_REGISTER(BSS_ID0);
	AR5K_PRINT_REGISTER(SLOT_TIME);
	AR5K_PRINT_REGISTER(TIME_OUT);
	AR5K_PRINT_REGISTER(RSSI_THR);
	AR5K_PRINT_REGISTER(BEACON);
	AR5K_PRINT_REGISTER(CFP_PERIOD);
	AR5K_PRINT_REGISTER(TIMER0);
	AR5K_PRINT_REGISTER(TIMER2);
	AR5K_PRINT_REGISTER(TIMER3);
	AR5K_PRINT_REGISTER(CFP_DUR);
	AR5K_PRINT_REGISTER(MCAST_FIL0);
	AR5K_PRINT_REGISTER(MCAST_FIL1);
	AR5K_PRINT_REGISTER(DIAG_SW);
	AR5K_PRINT_REGISTER(TSF_U32);
	AR5K_PRINT_REGISTER(ADDAC_TEST);
	AR5K_PRINT_REGISTER(DEFAULT_ANTENNA);
	AR5K_PRINT_REGISTER(LAST_TSTP);
	AR5K_PRINT_REGISTER(NAV);
	AR5K_PRINT_REGISTER(RTS_OK);
	AR5K_PRINT_REGISTER(ACK_FAIL);
	AR5K_PRINT_REGISTER(FCS_FAIL);
	AR5K_PRINT_REGISTER(BEACON_CNT);
	AR5K_PRINT_REGISTER(TSF_PARM);
	AR5K_PRINT_REGISTER(RATE_DUR_0);
	AR5K_PRINT_REGISTER(KEYTABLE_0);
	IOLog("\n");

	IOLog("PHY registers:\n");
	AR5K_PRINT_REGISTER(PHY_TURBO);
	AR5K_PRINT_REGISTER(PHY_AGC);
	AR5K_PRINT_REGISTER(PHY_TIMING_3);
	AR5K_PRINT_REGISTER(PHY_CHIP_ID);
	AR5K_PRINT_REGISTER(PHY_AGCCTL);
	AR5K_PRINT_REGISTER(PHY_NF);
	AR5K_PRINT_REGISTER(PHY_SCR);
	AR5K_PRINT_REGISTER(PHY_SLMT);
	AR5K_PRINT_REGISTER(PHY_SCAL);
	AR5K_PRINT_REGISTER(PHY_RX_DELAY);
	AR5K_PRINT_REGISTER(PHY_IQ);
	AR5K_PRINT_REGISTER(PHY_PAPD_PROBE);
	AR5K_PRINT_REGISTER(PHY_TXPOWER_RATE1);
	AR5K_PRINT_REGISTER(PHY_TXPOWER_RATE2);
	AR5K_PRINT_REGISTER(PHY_FC);
	AR5K_PRINT_REGISTER(PHY_RADAR);
	AR5K_PRINT_REGISTER(PHY_ANT_SWITCH_TABLE_0);
	AR5K_PRINT_REGISTER(PHY_ANT_SWITCH_TABLE_1);
	IOLog("\n");
#endif
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_get_diag_state(int id, void **device,
    u_int *size)
{
	/*
	 * We'll ignore this right now. This seems to be some kind of an obscure
	 * debugging interface for the binary-only HAL.
	 */
	return (AH_FALSE);
}

void
OpenHAL5212::ar5k_ar5212_get_lladdr(u_int8_t *mac)
{
	bcopy(ah_sta_id, mac, IEEE80211_ADDR_LEN);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_lladdr(const u_int8_t *mac)
{
	u_int32_t low_id, high_id;

	/* Set new station ID */
	bcopy(mac, ah_sta_id, IEEE80211_ADDR_LEN);

	bcopy(mac, &low_id, 4);
	bcopy(mac + 4, &high_id, 2);
	high_id = 0x0000ffff & OSSwapHostToLittleInt(high_id);

	AR5K_REG_WRITE(AR5K_AR5212_STA_ID0, low_id);
	AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, OSSwapLittleToHostInt(high_id));

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_regdomain(u_int16_t regdomain,
    HAL_STATUS *status)
{
	ieee80211_regdomain_t ieee_regdomain;

	ieee_regdomain = ar5k_regdomain_to_ieee(regdomain);

	if (ar5k_eeprom_regulation_domain(AH_TRUE,
		&ieee_regdomain) == AH_TRUE) {
		*status = HAL_OK;
		return (AH_TRUE);
	}

	*status = HAL_EIO;

	return (AH_FALSE);
}

void
OpenHAL5212::ar5k_ar5212_set_ledstate(HAL_LED_STATE state)
{
	u_int32_t led;

	AR5K_REG_DISABLE_BITS(AR5K_AR5212_PCICFG,
	    AR5K_AR5212_PCICFG_LEDMODE |  AR5K_AR5212_PCICFG_LED);

	/*
	 * Some blinking values, define at your wish
	 */
	switch (state) {
	case IEEE80211_S_SCAN:
	case IEEE80211_S_AUTH:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_PEND;
		break;

	case IEEE80211_S_INIT:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_NONE;
		break;

	case IEEE80211_S_ASSOC:
	case IEEE80211_S_RUN:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_ASSOC;
		break;

	default:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROM |
		    AR5K_AR5212_PCICFG_LED_NONE;
		break;
	}

	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PCICFG, led);
}

void
OpenHAL5212::ar5k_ar5212_set_associd(const u_int8_t *bssid,
    u_int16_t assoc_id, u_int16_t tim_offset)
{
	u_int32_t low_id, high_id;

	/*
	 * Set simple BSSID mask
	 */
	AR5K_REG_WRITE(AR5K_AR5212_BSS_IDM0, 0xfffffff);
	AR5K_REG_WRITE(AR5K_AR5212_BSS_IDM1, 0xfffffff);

	/*
	 * Set BSSID which triggers the "SME Join" operation
	 */
	bcopy(bssid, &low_id, 4);
	bcopy(bssid + 4, &high_id, 2);
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID0, OSSwapHostToLittleInt32(low_id));
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID1, OSSwapHostToLittleInt32(high_id) |
	    ((assoc_id & 0x3fff) << AR5K_AR5212_BSS_ID1_AID_S));
	bcopy(bssid, &ah_bssid, IEEE80211_ADDR_LEN);

	if (assoc_id == 0) {
		ar5k_ar5212_disable_pspoll();
		return;
	}

	AR5K_REG_WRITE(AR5K_AR5212_BEACON,
	    (AR5K_REG_READ(AR5K_AR5212_BEACON) &
	    ~AR5K_AR5212_BEACON_TIM) |
	    (((tim_offset ? tim_offset + 4 : 0) <<
	    AR5K_AR5212_BEACON_TIM_S) &
	    AR5K_AR5212_BEACON_TIM));

	ar5k_ar5212_enable_pspoll(NULL, 0);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_gpio_output(u_int32_t gpio)
{
	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (AH_FALSE);

	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR,
	    (AR5K_REG_READ(AR5K_AR5212_GPIOCR) &~ AR5K_AR5212_GPIOCR_ALL(gpio))
	    | AR5K_AR5212_GPIOCR_ALL(gpio));

	return (AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_gpio_input(u_int32_t gpio)
{
	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (AH_FALSE);

	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR,
	    (AR5K_REG_READ(AR5K_AR5212_GPIOCR) &~ AR5K_AR5212_GPIOCR_ALL(gpio))
	    | AR5K_AR5212_GPIOCR_NONE(gpio));

	return (AH_TRUE);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_gpio(u_int32_t gpio)
{
	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (0xffffffff);

	/* GPIO input magic */
	return (((AR5K_REG_READ(AR5K_AR5212_GPIODI) &
	    AR5K_AR5212_GPIODI_M) >> gpio) & 0x1);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_gpio(u_int32_t gpio, u_int32_t val)
{
	u_int32_t data;

	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (AH_FALSE);

	/* GPIO output magic */
	data =  AR5K_REG_READ(AR5K_AR5212_GPIODO);

	data &= ~(1 << gpio);
	data |= (val&1) << gpio;

	AR5K_REG_WRITE(AR5K_AR5212_GPIODO, data);

	return (AH_TRUE);
}

void
OpenHAL5212::ar5k_ar5212_set_gpio_intr(u_int gpio,
    u_int32_t interrupt_level)
{
	u_int32_t data;

	if (gpio > AR5K_AR5212_NUM_GPIO)
		return;

	/*
	 * Set the GPIO interrupt
	 */
	data = (AR5K_REG_READ(AR5K_AR5212_GPIOCR) &
	    ~(AR5K_AR5212_GPIOCR_INT_SEL(gpio) | AR5K_AR5212_GPIOCR_INT_SELH |
	    AR5K_AR5212_GPIOCR_INT_ENA | AR5K_AR5212_GPIOCR_ALL(gpio))) |
	    (AR5K_AR5212_GPIOCR_INT_SEL(gpio) | AR5K_AR5212_GPIOCR_INT_ENA);

	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR,
	    interrupt_level ? data : (data | AR5K_AR5212_GPIOCR_INT_SELH));

	ah_imr |= AR5K_AR5212_PIMR_GPIO;

	/* Enable GPIO interrupts */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PIMR, AR5K_AR5212_PIMR_GPIO);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_tsf32()
{
	return (AR5K_REG_READ(AR5K_AR5212_TSF_L32));
}

u_int64_t
OpenHAL5212::ar5k_ar5212_get_tsf64()
{
	u_int64_t tsf = AR5K_REG_READ(AR5K_AR5212_TSF_U32);

	return (AR5K_REG_READ(AR5K_AR5212_TSF_L32) | (tsf << 32));
}

void
OpenHAL5212::ar5k_ar5212_reset_tsf()
{
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_BEACON,
	    AR5K_AR5212_BEACON_RESET_TSF);
}

u_int16_t
OpenHAL5212::ar5k_ar5212_get_regdomain()
{
	return (ar5k_get_regdomain());
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_detect_card_present()
{
	u_int16_t magic;

	/*
	 * Checking the EEPROM's magic value could be an indication
	 * if the card is still present. I didn't find another suitable
	 * way to do this.
	 */
	if (ar5k_ar5212_eeprom_read(AR5K_EEPROM_MAGIC, &magic) != 0)
		return (AH_FALSE);

	return (magic == AR5K_EEPROM_MAGIC_VALUE ? AH_TRUE : AH_FALSE);
}

void
OpenHAL5212::ar5k_ar5212_update_mib_counters(HAL_MIB_STATS *statistics)
{
	/* Read-And-Clear */
	statistics->ackrcv_bad += AR5K_REG_READ(AR5K_AR5212_ACK_FAIL);
	statistics->rts_bad += AR5K_REG_READ(AR5K_AR5212_RTS_FAIL);
	statistics->rts_good += AR5K_REG_READ(AR5K_AR5212_RTS_OK);
	statistics->fcs_bad += AR5K_REG_READ(AR5K_AR5212_FCS_FAIL);
	statistics->beacons += AR5K_REG_READ(AR5K_AR5212_BEACON_CNT);

	/* Reset profile count registers */
	AR5K_REG_WRITE(AR5K_AR5212_PROFCNT_TX, 0);
	AR5K_REG_WRITE(AR5K_AR5212_PROFCNT_RX, 0);
	AR5K_REG_WRITE(AR5K_AR5212_PROFCNT_RXCLR, 0);
	AR5K_REG_WRITE(AR5K_AR5212_PROFCNT_CYCLE, 0);
}

HAL_RFGAIN
OpenHAL5212::ar5k_ar5212_get_rf_gain()
{
	u_int32_t data, type;

	if ((ah_rf_banks == NULL) || (!ah_gain.g_active))
		return (HAL_RFGAIN_INACTIVE);

	if (ah_rf_gain != HAL_RFGAIN_READ_REQUESTED)
		goto done;

	data = AR5K_REG_READ(AR5K_AR5212_PHY_PAPD_PROBE);

	if (!(data & AR5K_AR5212_PHY_PAPD_PROBE_TX_NEXT)) {
		ah_gain.g_current =
		    data >> AR5K_AR5212_PHY_PAPD_PROBE_GAINF_S;
		type = AR5K_REG_MS(data, AR5K_AR5212_PHY_PAPD_PROBE_TYPE);

		if (type == AR5K_AR5212_PHY_PAPD_PROBE_TYPE_CCK)
			ah_gain.g_current += AR5K_GAIN_CCK_PROBE_CORR;

		if (ah_radio == AR5K_AR5112) {
			ar5k_rfregs_gainf_corr();
			ah_gain.g_current =
			    ah_gain.g_current >= ah_gain.g_f_corr ?
			    (ah_gain.g_current - ah_gain.g_f_corr) :
			    0;
		}

		if (ar5k_rfregs_gain_readback() &&
		    AR5K_GAIN_CHECK_ADJUST(&ah_gain) &&
		    ar5k_rfregs_gain_adjust())
			ah_rf_gain = HAL_RFGAIN_NEED_CHANGE;
	}

 done:
	return (ah_rf_gain);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_slot_time(u_int slot_time)
{
	if (slot_time < HAL_SLOT_TIME_9 || slot_time > HAL_SLOT_TIME_MAX)
		return (AH_FALSE);

	AR5K_REG_WRITE(AR5K_AR5212_DCU_GBL_IFS_SLOT, slot_time);

	return (AH_TRUE);
}

u_int
OpenHAL5212::ar5k_ar5212_get_slot_time()
{
	return (AR5K_REG_READ(AR5K_AR5212_DCU_GBL_IFS_SLOT) & 0xffff);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_ack_timeout(u_int timeout)
{
	if (ar5k_clocktoh(AR5K_REG_MS(0xffffffff, AR5K_AR5212_TIME_OUT_ACK),
	    ah_turbo) <= timeout)
		return (AH_FALSE);

	AR5K_REG_WRITE_BITS(AR5K_AR5212_TIME_OUT, AR5K_AR5212_TIME_OUT_ACK,
	    ar5k_htoclock(timeout, ah_turbo));

	return (AH_TRUE);
}

u_int
OpenHAL5212::ar5k_ar5212_get_ack_timeout()
{
	return (ar5k_clocktoh(AR5K_REG_MS(AR5K_REG_READ(AR5K_AR5212_TIME_OUT),
	    AR5K_AR5212_TIME_OUT_ACK), ah_turbo));
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_cts_timeout(u_int timeout)
{
	if (ar5k_clocktoh(AR5K_REG_MS(0xffffffff, AR5K_AR5212_TIME_OUT_CTS),
	    ah_turbo) <= timeout)
		return (AH_FALSE);

	AR5K_REG_WRITE_BITS(AR5K_AR5212_TIME_OUT, AR5K_AR5212_TIME_OUT_CTS,
	    ar5k_htoclock(timeout, ah_turbo));

	return (AH_TRUE);
}

u_int
OpenHAL5212::ar5k_ar5212_get_cts_timeout()
{
	return (ar5k_clocktoh(AR5K_REG_MS(AR5K_REG_READ(AR5K_AR5212_TIME_OUT),
	    AR5K_AR5212_TIME_OUT_CTS), ah_turbo));
}

#pragma mark -

/*
 * Key table (WEP) functions
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_is_cipher_supported(HAL_CIPHER cipher)
{
	/*
	 * The AR5212 only supports WEP
	 */
	if (cipher == HAL_CIPHER_WEP)
		return (AH_TRUE);

	return (AH_FALSE);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_keycache_size()
{
	return (AR5K_AR5212_KEYCACHE_SIZE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_reset_key(u_int16_t entry)
{
	int i;

	AR5K_ASSERT_ENTRY(entry, AR5K_AR5212_KEYTABLE_SIZE);

	for (i = 0; i < AR5K_AR5212_KEYCACHE_SIZE; i++)
		AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_OFF(entry, i), 0);

	/* Set NULL encryption */
	AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_TYPE(entry),
	    AR5K_AR5212_KEYTABLE_TYPE_NULL);

	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_is_key_valid(u_int16_t entry)
{
	AR5K_ASSERT_ENTRY(entry, AR5K_AR5212_KEYTABLE_SIZE);

	/*
	 * Check the validation flag at the end of the entry
	 */
	if (AR5K_REG_READ(AR5K_AR5212_KEYTABLE_MAC1(entry)) &
	    AR5K_AR5212_KEYTABLE_VALID)
		return (AH_TRUE);

	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_key(u_int16_t entry,
    const HAL_KEYVAL *keyval, const u_int8_t *mac, int xor_notused)
{
	int i;
	u_int32_t key_v[AR5K_AR5212_KEYCACHE_SIZE - 2];

	AR5K_ASSERT_ENTRY(entry, AR5K_AR5212_KEYTABLE_SIZE);

	bzero(&key_v, sizeof(key_v));

	switch (keyval->wk_len) {
	case AR5K_KEYVAL_LENGTH_40:
		bcopy(keyval->wk_key, &key_v[0], 4);
		bcopy(keyval->wk_key + 4, &key_v[1], 1);
		key_v[5] = AR5K_AR5212_KEYTABLE_TYPE_40;
		break;

	case AR5K_KEYVAL_LENGTH_104:
		bcopy(keyval->wk_key, &key_v[0], 4);
		bcopy(keyval->wk_key + 4, &key_v[1], 2);
		bcopy(keyval->wk_key + 6, &key_v[2], 4);
		bcopy(keyval->wk_key + 10, &key_v[3], 2);
		bcopy(keyval->wk_key + 12, &key_v[4], 1);
		key_v[5] = AR5K_AR5212_KEYTABLE_TYPE_104;
		break;

	case AR5K_KEYVAL_LENGTH_128:
		bcopy(keyval->wk_key, &key_v[0], 4);
		bcopy(keyval->wk_key + 4, &key_v[1], 2);
		bcopy(keyval->wk_key + 6, &key_v[2], 4);
		bcopy(keyval->wk_key + 10, &key_v[3], 2);
		bcopy(keyval->wk_key + 12, &key_v[4], 4);
		key_v[5] = AR5K_AR5212_KEYTABLE_TYPE_128;
		break;

	default:
		/* Unsupported key length (not WEP40/104/128) */
		return (AH_FALSE);
	}

	for (i = 0; i < AR5K_ELEMENTS(key_v); i++)
		AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_OFF(entry, i), key_v[i]);

	return (ar5k_ar5212_set_key_lladdr(entry, mac));
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_key_lladdr(u_int16_t entry,
    const u_int8_t *mac)
{
	u_int32_t low_id, high_id;
	const u_int8_t *mac_v;

	/*
	 * Invalid entry (key table overflow)
	 */
	AR5K_ASSERT_ENTRY(entry, AR5K_AR5212_KEYTABLE_SIZE);

	/* MAC may be NULL if it's a broadcast key */
	mac_v = mac == NULL ? etherbroadcastaddr : mac;

	bcopy(mac_v, &low_id, 4);
	bcopy(mac_v + 4, &high_id, 2);
	high_id |= AR5K_AR5212_KEYTABLE_VALID;

	AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_MAC0(entry), low_id);
	AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_MAC1(entry), high_id);

	return (AH_TRUE);
}

#pragma mark -

/*
 * Power management functions
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_set_power(HAL_POWER_MODE mode,
    HAL_BOOL set_chip, u_int16_t sleep_duration)
{
	u_int32_t staid;
	int i;

	staid = AR5K_REG_READ(AR5K_AR5212_STA_ID1);

	switch (mode) {
	case HAL_PM_AUTO:
		staid &= ~AR5K_AR5212_STA_ID1_DEFAULT_ANTENNA;
		/* fallthrough */
	case HAL_PM_NETWORK_SLEEP:
		if (set_chip == AH_TRUE) {
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE | sleep_duration);
		}
		staid |= AR5K_AR5212_STA_ID1_PWR_SV;
		break;

	case HAL_PM_FULL_SLEEP:
		if (set_chip == AH_TRUE) {
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE_SLP);
		}
		staid |= AR5K_AR5212_STA_ID1_PWR_SV;
		break;

	case HAL_PM_AWAKE:
		if (set_chip == AH_FALSE)
			goto commit;

		AR5K_REG_WRITE(AR5K_AR5212_SCR, AR5K_AR5212_SCR_SLE_WAKE);

		for (i = 5000; i > 0; i--) {
			/* Check if the AR5212 did wake up */
			if ((AR5K_REG_READ(AR5K_AR5212_PCICFG) &
			    AR5K_AR5212_PCICFG_SPWR_DN) == 0)
				break;

			/* Wait a bit and retry */
			AR5K_DELAY(200);
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE_WAKE);
		}

		/* Fail if the AR5212 didn't wake up */
		if (i <= 0)
			return (AH_FALSE);

		staid &= ~AR5K_AR5212_STA_ID1_PWR_SV;
		break;

	default:
		return (AH_FALSE);
	}

 commit:
	ah_power_mode = mode;

	AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, staid);

	return (AH_TRUE);
}

HAL_POWER_MODE
OpenHAL5212::ar5k_ar5212_get_power_mode()
{
	return (ah_power_mode);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_query_pspoll_support()
{
	/* nope */
	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_init_pspoll()
{
	/*
	 * Not used on the AR5212
	 */
	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_enable_pspoll(u_int8_t *bssid,
    u_int16_t assoc_id)
{
	return (AH_FALSE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_disable_pspoll()
{
	return (AH_FALSE);
}

#pragma mark -

/*
 * Beacon functions
 */

void
OpenHAL5212::ar5k_ar5212_init_beacon(u_int32_t next_beacon,
    u_int32_t interval)
{
	u_int32_t timer1, timer2, timer3;

	/*
	 * Set the additional timers by mode
	 */
	switch (ah_op_mode) {
	case HAL_M_STA:
		timer1 = 0x0000ffff;
		timer2 = 0x0007ffff;
		break;

	default:
		timer1 = (next_beacon - AR5K_TUNE_DMA_BEACON_RESP) <<
		    0x00000003;
		timer2 = (next_beacon - AR5K_TUNE_SW_BEACON_RESP) <<
		    0x00000003;
	}

	timer3 = next_beacon +
	    (ah_atim_window ? ah_atim_window : 1);

	/*
	 * Enable all timers and set the beacon register
	 * (next beacon, DMA beacon, software beacon, ATIM window time)
	 */
	AR5K_REG_WRITE(AR5K_AR5212_TIMER0, next_beacon);
	AR5K_REG_WRITE(AR5K_AR5212_TIMER1, timer1);
	AR5K_REG_WRITE(AR5K_AR5212_TIMER2, timer2);
	AR5K_REG_WRITE(AR5K_AR5212_TIMER3, timer3);

	AR5K_REG_WRITE(AR5K_AR5212_BEACON, interval &
	    (AR5K_AR5212_BEACON_PERIOD | AR5K_AR5212_BEACON_RESET_TSF |
	    AR5K_AR5212_BEACON_ENABLE));
}

void
OpenHAL5212::ar5k_ar5212_set_beacon_timers(
    const HAL_BEACON_STATE *state, u_int32_t tsf, u_int32_t dtim_count,
    u_int32_t cfp_count)
{
	u_int32_t cfp_period, next_cfp, dtim, interval, next_beacon;

	/* Return on an invalid beacon state */
	if (state->bs_interval < 1)
		return;

	interval = state->bs_intval;
	dtim = state->bs_dtimperiod;

	/*
	 * PCF support?
	 */
	if (state->bs_cfp_period > 0) {
		/* Enable CFP mode and set the CFP and timer registers */
		cfp_period = state->bs_cfp_period * state->bs_dtim_period *
		    state->bs_interval;
		next_cfp = (cfp_count * state->bs_dtim_period + dtim_count) *
		    state->bs_interval;

		AR5K_REG_ENABLE_BITS(AR5K_AR5212_STA_ID1,
		    AR5K_AR5212_STA_ID1_PCF);
		AR5K_REG_WRITE(AR5K_AR5212_CFP_PERIOD, cfp_period);
		AR5K_REG_WRITE(AR5K_AR5212_CFP_DUR, state->bs_cfp_max_duration);
		AR5K_REG_WRITE(AR5K_AR5212_TIMER2,
		    (tsf + (next_cfp == 0 ? cfp_period : next_cfp)) << 3);
	} else {
		/* Disable PCF mode */
		AR5K_REG_DISABLE_BITS(AR5K_AR5212_STA_ID1,
		    AR5K_AR5212_STA_ID1_PCF);
	}

	/*
	 * Enable the beacon timer register
	 */
	AR5K_REG_WRITE(AR5K_AR5212_TIMER0, state->bs_next_beacon);

	/*
	 * Start the beacon timers
	 */
	AR5K_REG_WRITE(AR5K_AR5212_BEACON,
	    (AR5K_REG_READ(AR5K_AR5212_BEACON) &~
	    (AR5K_AR5212_BEACON_PERIOD | AR5K_AR5212_BEACON_TIM)) |
	    AR5K_REG_SM(state->bs_tim_offset ? state->bs_tim_offset + 4 : 0,
	    AR5K_AR5212_BEACON_TIM) | AR5K_REG_SM(state->bs_interval,
	    AR5K_AR5212_BEACON_PERIOD));

	/*
	 * Write new beacon miss threshold, if it appears to be valid
	 */
	if ((AR5K_AR5212_RSSI_THR_BMISS >> AR5K_AR5212_RSSI_THR_BMISS_S) <
	    state->bs_bmiss_threshold)
		return;

	AR5K_REG_WRITE_BITS(AR5K_AR5212_RSSI_THR_M,
	    AR5K_AR5212_RSSI_THR_BMISS, state->bs_bmiss_threshold);

	/*
	 * Set sleep registers
	 */
	if ((state->bs_sleepduration > state->bs_interval) &&
	    (roundup(state->bs_sleepduration, interval) ==
	    state->bs_sleepduration))
		interval = state->bs_sleepduration;

	if (state->bs_sleepduration > dtim &&
	    (dtim == 0 || roundup(state->bs_sleepduration, dtim) ==
	    state->bs_sleepduration))
		dtim = state->bs_sleepduration;

	if (interval > dtim)
		return;

	next_beacon = interval == dtim ?
	    state->bs_nextdtim: state->bs_nexttbtt;

	AR5K_REG_WRITE(AR5K_AR5212_SLEEP0,
	    AR5K_REG_SM((state->bs_nextdtim - 3) << 3,
	    AR5K_AR5212_SLEEP0_NEXT_DTIM) |
	    AR5K_REG_SM(10, AR5K_AR5212_SLEEP0_CABTO) |
	    AR5K_AR5212_SLEEP0_ENH_SLEEP_EN |
	    AR5K_AR5212_SLEEP0_ASSUME_DTIM);
	AR5K_REG_WRITE(AR5K_AR5212_SLEEP1,
	    AR5K_REG_SM((next_beacon - 3) << 3,
	    AR5K_AR5212_SLEEP1_NEXT_TIM) |
	    AR5K_REG_SM(10, AR5K_AR5212_SLEEP1_BEACON_TO));
	AR5K_REG_WRITE(AR5K_AR5212_SLEEP2,
	    AR5K_REG_SM(interval, AR5K_AR5212_SLEEP2_TIM_PER) |
	    AR5K_REG_SM(dtim, AR5K_AR5212_SLEEP2_DTIM_PER));
}

void
OpenHAL5212::ar5k_ar5212_reset_beacon()
{
	/*
	 * Disable beacon timer
	 */
	AR5K_REG_WRITE(AR5K_AR5212_TIMER0, 0);

	/*
	 * Disable some beacon register values
	 */
	AR5K_REG_DISABLE_BITS(AR5K_AR5212_STA_ID1,
	    AR5K_AR5212_STA_ID1_DEFAULT_ANTENNA | AR5K_AR5212_STA_ID1_PCF);
	AR5K_REG_WRITE(AR5K_AR5212_BEACON, AR5K_AR5212_BEACON_PERIOD);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_wait_for_beacon(bus_addr_t phys_addr)
{
	HAL_BOOL ret;

	/*
	 * Wait for beaconn queue to be done
	 */
	ret = ar5k_register_timeout(
	    AR5K_AR5212_QCU_STS(HAL_TX_QUEUE_ID_BEACON),
	    AR5K_AR5212_QCU_STS_FRMPENDCNT, 0, AH_FALSE);

	if (AR5K_REG_READ_Q(AR5K_AR5212_QCU_TXE, HAL_TX_QUEUE_ID_BEACON))
		return (AH_FALSE);

	return (ret);
}

#pragma mark -

/*
 * Interrupt handling
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_is_intr_pending()
{
	return (AR5K_REG_READ(AR5K_AR5212_INTPEND) == 0 ? AH_FALSE : AH_TRUE);
}

HAL_BOOL
OpenHAL5212::ar5k_ar5212_get_isr(u_int32_t *interrupt_mask)
{
	u_int32_t data;

	/*
	 * Read interrupt status from the Read-And-Clear shadow register
	 */
	data = AR5K_REG_READ(AR5K_AR5212_RAC_PISR);

	/*
	 * Get abstract interrupt mask (HAL-compatible)
	 */
	*interrupt_mask = (data & HAL_INT_COMMON) & ah_imr;

	if (data == HAL_INT_NOCARD)
		return (AH_FALSE);

	if (data & (AR5K_AR5212_PISR_RXOK | AR5K_AR5212_PISR_RXERR))
		*interrupt_mask |= HAL_INT_RX;

	if (data & (AR5K_AR5212_PISR_TXOK | AR5K_AR5212_PISR_TXERR))
		*interrupt_mask |= HAL_INT_TX;

	if (data & (AR5K_AR5212_PISR_HIUERR))
		*interrupt_mask |= HAL_INT_FATAL;

	/*
	 * Special interrupt handling (not caught by the driver)
	 */
	if (((*interrupt_mask) & AR5K_AR5212_PISR_RXPHY) &&
	    ah_radar.r_enabled == AH_TRUE)
		ar5k_radar_alert();

	if (*interrupt_mask == 0)
		AR5K_PRINTF("0x%08x\n", data);

	return (AH_TRUE);
}

u_int32_t
OpenHAL5212::ar5k_ar5212_get_intr()
{
	/* Return the interrupt mask stored previously */
	return (ah_imr);
}

HAL_INT
OpenHAL5212::ar5k_ar5212_set_intr(HAL_INT new_mask)
{
	HAL_INT old_mask, int_mask;

	/*
	 * Disable card interrupts to prevent any race conditions
	 * (they will be re-enabled afterwards).
	 */
	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_DISABLE);

	old_mask = ah_imr;

	/*
	 * Add additional, chipset-dependent interrupt mask flags
	 * and write them to the IMR (interrupt mask register).
	 */
	int_mask = new_mask & HAL_INT_COMMON;

	if (new_mask & HAL_INT_RX)
		int_mask |=
		    AR5K_AR5212_PIMR_RXOK |
		    AR5K_AR5212_PIMR_RXERR |
		    AR5K_AR5212_PIMR_RXORN |
		    AR5K_AR5212_PIMR_RXDESC;

	if (new_mask & HAL_INT_TX)
		int_mask |=
		    AR5K_AR5212_PIMR_TXOK |
		    AR5K_AR5212_PIMR_TXERR |
		    AR5K_AR5212_PIMR_TXDESC |
		    AR5K_AR5212_PIMR_TXURN;

	if (new_mask & HAL_INT_FATAL) {
		int_mask |= AR5K_AR5212_PIMR_HIUERR;
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_SIMR2,
		    AR5K_AR5212_SIMR2_MCABT |
		    AR5K_AR5212_SIMR2_SSERR |
		    AR5K_AR5212_SIMR2_DPERR);
	}

	AR5K_REG_WRITE(AR5K_AR5212_PIMR, int_mask);

	/* Store new interrupt mask */
	ah_imr = new_mask;

	/* ..re-enable interrupts */
	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_ENABLE);

	return (old_mask);
}

#pragma mark -

/*
 * Misc internal functions
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_get_capabilities()
{
	u_int16_t ee_header;

	/* Capabilities stored in the EEPROM */
	ee_header = ah_capabilities.cap_eeprom.ee_header;

	/*
	 * XXX The AR5212 tranceiver supports frequencies from 4920 to 6100GHz
	 * XXX and from 2312 to 2732GHz. There are problems with the current
	 * XXX ieee80211 implementation because the IEEE channel mapping
	 * XXX does not support negative channel numbers (2312MHz is channel
	 * XXX -19). Of course, this doesn't matter because these channels
	 * XXX are out of range but some regulation domains like MKK (Japan)
	 * XXX will support frequencies somewhere around 4.8GHz.
	 */

	/*
	 * Set radio capabilities
	 */

	if (AR5K_EEPROM_HDR_11A(ee_header)) {
		ah_capabilities.cap_range.range_5ghz_min = 5005; /* 4920 */
		ah_capabilities.cap_range.range_5ghz_max = 6100;

		/* Set supported modes */
		ah_capabilities.cap_mode =
		    HAL_MODE_11A | HAL_MODE_TURBO | HAL_MODE_XR;
	}

	/* This chip will support 802.11b if the 2GHz radio is connected */
	if (AR5K_EEPROM_HDR_11B(ee_header) || AR5K_EEPROM_HDR_11G(ee_header)) {
		ah_capabilities.cap_range.range_2ghz_min = 2412; /* 2312 */
		ah_capabilities.cap_range.range_2ghz_max = 2732;
		ah_capabilities.cap_mode |= HAL_MODE_11B;

		if (AR5K_EEPROM_HDR_11B(ee_header))
			ah_capabilities.cap_mode |= HAL_MODE_11B;
		if (AR5K_EEPROM_HDR_11G(ee_header))
			ah_capabilities.cap_mode |= HAL_MODE_11G;
	}

	/* GPIO */
	ah_gpio_npins = AR5K_AR5212_NUM_GPIO;

	/* Set number of supported TX queues */
	ah_capabilities.cap_queues.q_tx_num = AR5K_AR5212_TX_NUM_QUEUES;

	return (AH_TRUE);
}

void
OpenHAL5212::ar5k_ar5212_radar_alert(HAL_BOOL enable)
{
	/*
	 * Enable radar detection
	 */
	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_DISABLE);

	if (enable == AH_TRUE) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_RADAR,
		    AR5K_AR5212_PHY_RADAR_ENABLE);
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_PIMR,
		    AR5K_AR5212_PIMR_RXPHY);
	} else {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_RADAR,
		    AR5K_AR5212_PHY_RADAR_DISABLE);
		AR5K_REG_DISABLE_BITS(AR5K_AR5212_PIMR,
		    AR5K_AR5212_PIMR_RXPHY);
	}

	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_ENABLE);
}

#pragma mark -

/*
 * EEPROM access functions
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_eeprom_is_busy()
{
	return (AR5K_REG_READ(AR5K_AR5212_CFG) & AR5K_AR5212_CFG_EEBS ?
	    AH_TRUE : AH_FALSE);
}

HAL_STATUS
OpenHAL5212::ar5k_ar5212_eeprom_read(u_int32_t offset, u_int16_t *data)
{
	u_int32_t status, i;

	/*
	 * Initialize EEPROM access
	 */
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_BASE, (u_int8_t)offset);
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_EEPROM_CMD,
	    AR5K_AR5212_EEPROM_CMD_READ);

	for (i = AR5K_TUNE_REGISTER_TIMEOUT; i > 0; i--) {
		status = AR5K_REG_READ(AR5K_AR5212_EEPROM_STATUS);
		if (status & AR5K_AR5212_EEPROM_STAT_RDDONE) {
			if (status & AR5K_AR5212_EEPROM_STAT_RDERR)
				return (HAL_EIO);
			*data = (u_int16_t)
			    (AR5K_REG_READ(AR5K_AR5212_EEPROM_DATA) & 0xffff);
			return (HAL_OK);
		}
		AR5K_DELAY(15);
	}

	return (HAL_EIO);
}

HAL_STATUS
OpenHAL5212::ar5k_ar5212_eeprom_write(u_int32_t offset, u_int16_t data)
{
	u_int32_t status, timeout;

	/*
	 * Prime write pump
	 */
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_BASE, offset);
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_DATA, data);
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_CMD, AR5K_AR5212_EEPROM_CMD_WRITE);

	for (timeout = 10000; timeout > 0; timeout--) {
		AR5K_DELAY(1);
		status = AR5K_REG_READ(AR5K_AR5212_EEPROM_STATUS);
		if (status & AR5K_AR5212_EEPROM_STAT_WRDONE) {
			if (status & AR5K_AR5212_EEPROM_STAT_WRERR)
				return (HAL_EIO);
			return (HAL_OK);
		}
	}

	return (HAL_EEWRITE);
}

#pragma mark -

/*
 * TX power setup
 */

HAL_BOOL
OpenHAL5212::ar5k_ar5212_txpower(HAL_CHANNEL *channel, u_int txpower)
{
	HAL_BOOL tpc = ah_txpower.txp_tpc;
	int i;

	if (txpower > AR5K_TUNE_MAX_TXPOWER) {
		AR5K_PRINTF("invalid tx power: %u\n", txpower);
		return (AH_FALSE);
	}

	/* Reset TX power values */
	bzero(&ah_txpower, sizeof(ah_txpower));
	ah_txpower.txp_tpc = tpc;

	/* Initialize TX power table */
	ar5k_txpower_table(channel, txpower);

	/* 
	 * Write TX power values
	 */
	for (i = 0; i < (AR5K_EEPROM_POWER_TABLE_SIZE / 2); i++) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_PCDAC_TXPOWER(i),
		    ((((ah_txpower.txp_pcdac[(i << 1) + 1] << 8) | 0xff) &
		    0xffff) << 16) | (((ah_txpower.txp_pcdac[i << 1] << 8)
		    | 0xff) & 0xffff));
	}

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE1,
	    AR5K_TXPOWER_OFDM(3, 24) | AR5K_TXPOWER_OFDM(2, 16)
	    | AR5K_TXPOWER_OFDM(1, 8) | AR5K_TXPOWER_OFDM(0, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE2,
	    AR5K_TXPOWER_OFDM(7, 24) | AR5K_TXPOWER_OFDM(6, 16)
	    | AR5K_TXPOWER_OFDM(5, 8) | AR5K_TXPOWER_OFDM(4, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE3,
	    AR5K_TXPOWER_CCK(10, 24) | AR5K_TXPOWER_CCK(9, 16)
	    | AR5K_TXPOWER_CCK(15, 8) | AR5K_TXPOWER_CCK(8, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE4,
	    AR5K_TXPOWER_CCK(14, 24) | AR5K_TXPOWER_CCK(13, 16)
	    | AR5K_TXPOWER_CCK(12, 8) | AR5K_TXPOWER_CCK(11, 0));

	if (ah_txpower.txp_tpc == AH_TRUE) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE_MAX,
		    AR5K_AR5212_PHY_TXPOWER_RATE_MAX_TPC_ENABLE |
		    AR5K_TUNE_MAX_TXPOWER);
	} else {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE_MAX,
		    AR5K_AR5212_PHY_TXPOWER_RATE_MAX |
		    AR5K_TUNE_MAX_TXPOWER);
	}

	return (AH_TRUE);
}
