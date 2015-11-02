/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2015 The srsUE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "upper/usim.h"
#include "liblte_security.h"

using namespace srslte;

namespace srsue{

usim::usim()
{}

void usim::init(std::string imsi_, std::string imei_, std::string k_,
                std::string auth_algo_, std::string op_, std::string amf_,
                srslte::log *usim_log_)
{
  usim_log = usim_log_;

  const char *imsi_str = imsi_.c_str();
  const char *imei_str = imei_.c_str();
  uint32_t    i;

  if(32   == op_.length()   &&
     4    == amf_.length()  &&
     15   == imsi_.length() &&
     15   == imei_.length() &&
     32   == k_.length())
  {
    str_to_hex(op_, op);
    str_to_hex(amf_, amf);
    str_to_hex(k_, k);

    imsi = 0;
    for(i=0; i<15; i++)
    {
      imsi *= 10;
      imsi += imsi_str[i] - '0';
    }

    imei = 0;
    for(i=0; i<15; i++)
    {
      imei *= 10;
      imei += imei_str[i] - '0';
    }
  }

  auth_algo = auth_algo_milenage;
  if("xor" == auth_algo_) {
    auth_algo = auth_algo_xor;
  }
}

void usim::stop()
{}

/*******************************************************************************
  NAS interface
*******************************************************************************/

void usim::get_imsi_vec(uint8_t* imsi_, uint32_t n)
{
  if(NULL == imsi_ || n < 15)
  {
    usim_log->error("Invalid parameters to get_imsi_vec");
    return;
  }

  uint64_t temp = imsi;
  for(int i=14;i>=0;i--)
  {
      imsi_[i] = temp % 10;
      temp /= 10;
  }
}

void usim::get_imei_vec(uint8_t* imei_, uint32_t n)
{
  if(NULL == imei_ || n < 15)
  {
    usim_log->error("Invalid parameters to get_imei_vec");
    return;
  }

  uint64 temp = imei;
  for(int i=14;i>=0;i--)
  {
      imei_[i] = temp % 10;
      temp /= 10;
  }
}

void usim::generate_authentication_response(uint8_t  *rand,
                                            uint8_t  *autn_enb,
                                            uint16_t  mcc,
                                            uint16_t  mnc,
                                            bool     *net_valid,
                                            uint8_t  *res)
{
  if(auth_algo_xor == auth_algo) {
    gen_auth_res_xor(rand, autn_enb, mcc, mnc, net_valid, res);
  } else {
    gen_auth_res_milenage(rand, autn_enb, mcc, mnc, net_valid, res);
  }
}

void usim::gen_auth_res_milenage( uint8_t  *rand,
                                  uint8_t  *autn_enb,
                                  uint16_t  mcc,
                                  uint16_t  mnc,
                                  bool     *net_valid,
                                  uint8_t  *res)
{
  uint32_t i;
  uint8_t  sqn[6];

  *net_valid = true;

  // Use RAND and K to compute RES, CK, IK and AK
  liblte_security_milenage_f2345(k,
                                 op,
                                 rand,
                                 res,
                                 ck,
                                 ik,
                                 ak);

  // Extract sqn from autn
  for(i=0;i<6;i++)
  {
    sqn[i] = autn_enb[i] ^ ak[i];
  }

  // Generate MAC
  liblte_security_milenage_f1(k,
                              op,
                              rand,
                              sqn,
                              amf,
                              mac);

  // Construct AUTN
  for(i=0; i<6; i++)
  {
    autn[i] = sqn[i] ^ ak[i];
  }
  for(i=0; i<2; i++)
  {
    autn[6+i] = amf[i];
  }
  for(i=0; i<8; i++)
  {
    autn[8+i] = mac[i];
  }

  // Compare AUTNs
  for(i=0; i<16; i++)
  {
    if(autn[i] != autn_enb[i])
    {
      *net_valid = false;
    }
  }

  // Generate K_asme
  liblte_security_generate_k_asme(ck,
                                  ik,
                                  ak,
                                  sqn,
                                  mcc,
                                  mnc,
                                  k_asme);
}

// 3GPP TS 34.108 version 10.0.0 Section 8
void usim::gen_auth_res_xor(uint8_t  *rand,
                            uint8_t  *autn_enb,
                            uint16_t  mcc,
                            uint16_t  mnc,
                            bool     *net_valid,
                            uint8_t  *res)
{
  uint32_t i;
  uint8_t  sqn[6];
  uint8_t  xdout[16];
  uint8_t  cdout[8];

  *net_valid = true;

  // Use RAND and K to compute RES, CK, IK and AK
  for(i=0; i<16; i++) {
    xdout[i] = k[i]^rand[i];
  }
  for(i=0; i<16; i++) {
    res[i]  = xdout[i];
    ck[i]   = xdout[(i+1)%16];
    ik[i]   = xdout[(i+2)%16];
  }
  for(i=0; i<6; i++) {
    ak[i] = xdout[i+3];
  }

  // Extract sqn from autn
  for(i=0;i<6;i++) {
    sqn[i] = autn_enb[i] ^ ak[i];
  }

  // Generate cdout
  for(i=0; i<6; i++) {
    cdout[i] = sqn[i];
  }
  for(i=0; i<2; i++) {
    cdout[6+i] = amf[i];
  }

  // Generate MAC
  for(i=0;i<8;i++) {
    mac[i] = xdout[i] ^ cdout[i];
  }

  // Construct AUTN
  for(i=0; i<6; i++)
  {
    autn[i] = sqn[i] ^ ak[i];
  }
  for(i=0; i<2; i++)
  {
    autn[6+i] = amf[i];
  }
  for(i=0; i<8; i++)
  {
    autn[8+i] = mac[i];
  }

  // Compare AUTNs
  for(i=0; i<16; i++)
  {
    if(autn[i] != autn_enb[i])
    {
      *net_valid = false;
    }
  }

  // Generate K_asme
  liblte_security_generate_k_asme(ck,
                                  ik,
                                  ak,
                                  sqn,
                                  mcc,
                                  mnc,
                                  k_asme);
}

void usim::generate_nas_keys(uint8_t *k_nas_enc, uint8_t *k_nas_int)
{
  // Generate K_nas_enc and K_nas_int
  liblte_security_generate_k_nas(k_asme,
                                 LIBLTE_SECURITY_CIPHERING_ALGORITHM_ID_EEA0,
                                 LIBLTE_SECURITY_INTEGRITY_ALGORITHM_ID_128_EIA2,
                                 k_nas_enc,
                                 k_nas_int);
}

void usim::generate_as_keys(uint32_t count_ul, uint8_t *k_rrc_enc, uint8_t *k_rrc_int, uint8_t *k_up_enc, uint8_t *k_up_int)
{
  // Generate K_enb
  liblte_security_generate_k_enb(k_asme,
                                 count_ul,
                                 k_enb);

  // Generate K_rrc_enc and K_rrc_int
  liblte_security_generate_k_rrc(k_enb,
                                 LIBLTE_SECURITY_CIPHERING_ALGORITHM_ID_EEA0,
                                 LIBLTE_SECURITY_INTEGRITY_ALGORITHM_ID_128_EIA2,
                                 k_rrc_enc,
                                 k_rrc_int);

  // Generate K_up_enc and K_up_int
  liblte_security_generate_k_up(k_enb,
                                LIBLTE_SECURITY_CIPHERING_ALGORITHM_ID_EEA0,
                                LIBLTE_SECURITY_INTEGRITY_ALGORITHM_ID_128_EIA2,
                                k_up_enc,
                                k_up_int);
}

void usim::str_to_hex(std::string str, uint8_t *hex)
{
  uint32_t    i;
  const char *h_str   = str.c_str();
  uint32_t    len     = str.length();

  for(i=0; i<len/2; i++)
  {
    if(h_str[i*2+0] >= '0' && h_str[i*2+0] <= '9')
    {
      hex[i] = ( h_str[i*2+0] - '0') << 4;
    }else if( h_str[i*2+0] >= 'A' &&  h_str[i*2+0] <= 'F'){
      hex[i] = (( h_str[i*2+0] - 'A') + 0xA) << 4;
    }else{
      hex[i] = (( h_str[i*2+0] - 'a') + 0xA) << 4;
    }

    if( h_str[i*2+1] >= '0' &&  h_str[i*2+1] <= '9')
    {
      hex[i] |=  h_str[i*2+1] - '0';
    }else if( h_str[i*2+1] >= 'A' &&  h_str[i*2+1] <= 'F'){
      hex[i] |= ( h_str[i*2+1] - 'A') + 0xA;
    }else{
      hex[i] |= ( h_str[i*2+1] - 'a') + 0xA;
    }
  }
}

} // namespace srsue
