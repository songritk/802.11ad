/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR _OFDM PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "wifi-phy.h"
#include "sensitivity-model-60-ghz.h"
#include "ns3/log.h"
#include "ns3/interference-helper.h"
#include "sensitivity-lut.h"

NS_LOG_COMPONENT_DEFINE ("SensitivityModel60GHz");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SensitivityModel60GHz);

TypeId 
SensitivityModel60GHz::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SensitivityModel60GHz")
    .SetParent<ErrorRateModel> ()
    .AddConstructor<SensitivityModel60GHz> ()
    ;
  return tid;
}

SensitivityModel60GHz::SensitivityModel60GHz ()
{}

double 
SensitivityModel60GHz::GetChunkSuccessRate (WifiMode mode, double snr, uint32_t nbits) const
{
	NS_ASSERT_MSG(mode.GetModulationClass() == WIFI_MOD_CLASS_VHT_SC ||
	              mode.GetModulationClass() == WIFI_MOD_CLASS_VHT_OFDM,
			"Expecting 802.11ad VHT SC or OFDM modulation");
	std::string modename = mode.GetUniqueName();

	/*
	 * From TGad Draft 0.1 Table 67:
	 * 355 carriers total @5.15625MHz = 1.830468750 GHz
	 * Assume same for both OFDM and SC phys.
	 */
	uint64_t bandwidth = 1830468750;

	/* this is kinda silly, but convert from SNR back to RSS */
	double noise = 1.3803e-23 * 290.0 * bandwidth;
	/* Compute RSS in dBm, so add 30 from SNR */
	double rss = 10*log10(snr * noise) + 30;
	NS_LOG_FUNCTION(modename << "snr" << snr << "rss" << rss << "bits" << nbits);
	double rss_delta;
	double ber;
	/**** Control PHY ****/
	if (modename == "VHTMCS0_SC")
		rss_delta = rss - -78;
	/**** SC PHY ****/
	else if (modename == "VHTMCS1_SC")
		rss_delta = rss - -68;
	else if (modename == "VHTMCS2_SC")
		rss_delta = rss - -67;
	else if (modename == "VHTMCS3_SC")
		rss_delta = rss - -65;
	else if (modename == "VHTMCS4_SC")
		rss_delta = rss - -64;
	else if (modename == "VHTMCS5_SC")
		rss_delta = rss - -62;
	else if (modename == "VHTMCS6_SC")
		rss_delta = rss - -63;
	else if (modename == "VHTMCS7_SC")
		rss_delta = rss - -62;
	else if (modename == "VHTMCS8_SC")
		rss_delta = rss - -61;
	else if (modename == "VHTMCS9_SC")
		rss_delta = rss - -59;
	else if (modename == "VHTMCS10_SC")
		rss_delta = rss - -55;
	else if (modename == "VHTMCS11_SC")
		rss_delta = rss - -54;
	else if (modename == "VHTMCS12_SC")
		rss_delta = rss - -53;
	/**** OFDM PHY ****/
	else if (modename == "VHTMCS13_OFDM")
		rss_delta = rss - -66;
	else if (modename == "VHTMCS14_OFDM")
		rss_delta = rss - -64;
	else if (modename == "VHTMCS15_OFDM")
		rss_delta = rss - -63;
	else if (modename == "VHTMCS16_OFDM")
		rss_delta = rss - -62;
	else if (modename == "VHTMCS17_OFDM")
		rss_delta = rss - -60;
	else if (modename == "VHTMCS18_OFDM")
		rss_delta = rss - -58;
	else if (modename == "VHTMCS19_OFDM")
		rss_delta = rss - -56;
	else if (modename == "VHTMCS20_OFDM")
		rss_delta = rss - -54;
	else if (modename == "VHTMCS21_OFDM")
		rss_delta = rss - -53;
	else if (modename == "VHTMCS22_OFDM")
		rss_delta = rss - -51;
	else if (modename == "VHTMCS23_OFDM")
		rss_delta = rss - -49;
	else if (modename == "VHTMCS24_OFDM")
		rss_delta = rss - -47;
//	/**** Low power PHY ****/
//	else if (modename == "VHTMCS25a")
//		rss_delta = rss - -64;
//	else if (modename == "VHTMCS26a")
//		rss_delta = rss - -60;
//	else if (modename == "VHTMCS27a")
//		rss_delta = rss - -57;
	else
		NS_FATAL_ERROR("Unrecognized 60 GHz modulation");

	/* Compute BER in lookup table */
	if (rss_delta < -12.0)
		ber = sensitivity_ber(0);
	else if (rss_delta > 6.0)
		ber = sensitivity_ber(180);
	else
		ber = sensitivity_ber((int)(10*(rss_delta+12)));

	/* Compute PER from BER */
	return pow(1-ber, nbits);
}

} // namespace ns3
