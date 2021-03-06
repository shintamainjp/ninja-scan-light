/*
 * Copyright (c) 2013, M.Naruoka (fenrir)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the naruoka.org nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __ANALYZE_COMMON_H__
#define __ANALYZE_COMMON_H__

#include <iostream>
#include <fstream>
#include <map>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cfloat>
#if defined(_MSC_VER)
#include <io.h>
#include <fcntl.h>
#endif

#include "util/comstream.h"
#include "util/endian.h"

/**
 * Convert units from degrees to radians
 *
 * @param degrees
 * @return radians
 */
template <class FloatT>
FloatT deg2rad(const FloatT &degrees){return degrees * M_PI / 180;}
/**
 * Convert units from radians to degrees
 *
 * @param radians
 * @return degrees
 */
template <class FloatT>
FloatT rad2deg(const FloatT &radians){return radians * 180 / M_PI;}

template <class FloatT>
struct GlobalOptions {
  
  bool dump_update; ///< True for dumping states at time updates
  bool dump_correct; ///< True for dumping states at measurement updates
  bool has_initial_attitude; ///< Whether initial attitude is given.
  FloatT init_attitude_deg[3];   ///< Initial attitude [deg] (yaw, pitch, roll)
  FloatT start_gpstime;  ///< Start GPS time
  int start_gpswn; ///< Start GPS week
  FloatT end_gpstime;    ///< End GPS time
  int end_gpswn; ///< End GPS week
  bool est_bias; ///< True for performing bias estimation
  bool use_udkf; ///< True for UD Kalman filtering
  bool use_magnet; ///< True for utilizing magnetic sensor
  FloatT mag_heading_accuracy_deg; ///< Accuracy of magnetic sensor in degrees
  FloatT yaw_correct_with_mag_when_speed_less_than_ms; ///< Threshold for yaw compensation; performing it when under this value [m/s], or ignored non-positive values
  bool out_is_N_packet; ///< True for NPacket formatted outputs
  bool reduce_1pps_sync_error; ///< True when auto correction for 1pps sync. error is activated
  std::ostream *_out; ///< Pointer for output stream
  bool in_sylphide;   ///< True when inputs is Sylphide formated
  bool out_sylphide;  ///< True when outputs is Sylphide formated
  typedef std::map<const char *, std::iostream *> iostream_pool_t;
  iostream_pool_t iostream_pool;

  static const char *null_fname(){
#if defined(_MSC_VER)
    return "nul";
#else
    return "/dev/null";
#endif
  }
  
  GlobalOptions()
      : dump_update(true),
      dump_correct(false),
      has_initial_attitude(false),
      start_gpstime(0), end_gpstime(DBL_MAX),
      start_gpswn(0), end_gpswn(0),
      est_bias(true),
      use_udkf(false),
      use_magnet(false),
      mag_heading_accuracy_deg(3),
      yaw_correct_with_mag_when_speed_less_than_ms(5),
      out_is_N_packet(false),
      reduce_1pps_sync_error(true),
      _out(&(std::cout)),
      in_sylphide(false), out_sylphide(false),
      iostream_pool() {};
  virtual ~GlobalOptions(){
    for(int i(0); i < sizeof(init_attitude_deg) / sizeof(init_attitude_deg[0]); ++i){
      init_attitude_deg[i] = 0;
    }
    for(iostream_pool_t::iterator it(iostream_pool.begin());
        it != iostream_pool.end();
        ++it){
      it->second->flush();
      delete it->second;
    }
  }
  
  template <class T>
  bool is_time_in_range(const T &time){
    return (time >= start_gpstime) && (time <= end_gpstime);
  }
  
  void set_baudrate(ComportStream &com, const char *baudrate_spec){
    int baudrate(std::atoi(baudrate_spec));
    if(baudrate != com.buffer().set_baudrate(baudrate)){
      std::cerr << " => Unsupported baudrate!!" << std::endl;
      exit(-1);
    }
  }

#ifdef _WIN32
#define COMPORT_PREFIX "COM"
#else
#define COMPORT_PREFIX "/dev/tty"
#endif
  
  std::istream &spec2istream(
      const char *spec, 
      const bool force_fstream = false){
    if(!force_fstream){
      if(strcmp(spec, "-") == 0){
        // '-' stands for standard inputs
        std::cerr << "[std::cin]" << std::endl;
#if defined(_MSC_VER)
      setmode(fileno(stdin), O_BINARY);
#endif
        return std::cin;
      }else if(strstr(spec, COMPORT_PREFIX) == spec){
        std::cerr << spec << std::endl;
        // COM ports
        // COM_name[:baudrate] format is acceptable.
        char *baudrate_spec((char *)strchr(spec, ':'));
        if(baudrate_spec){
          *baudrate_spec = '\0';
          baudrate_spec++;
        }
        if(iostream_pool.find(spec) == iostream_pool.end()){
          ComportStream *com_in = new ComportStream(spec);
          if(baudrate_spec){set_baudrate(*com_in, baudrate_spec);}
          iostream_pool[spec] = com_in;
          return *com_in;
        }else{
          return *(iostream_pool[spec]);
        }
      }
    }
    
    std::cerr << spec;
    std::fstream *fin(new std::fstream(spec, std::ios::in | std::ios::binary));    
    if(fin->fail()){
      std::cerr << " => File not found!!" << std::endl;
      exit(-1);
    }
    std::cerr << std::endl;
    iostream_pool[spec] = fin;
    return *fin;
  }
  
  std::ostream &spec2ostream(
      const char *spec,
      const bool force_fstream = false){
    if(!force_fstream){
      if(std::strcmp(spec, "-") == 0){
        // '-' stands for standard outputs
        std::cerr << "[std::cout]" << std::endl;
#if defined(_MSC_VER)
      setmode(fileno(stdout), O_BINARY);
#endif
        return std::cout;
      }else if(std::strstr(spec, COMPORT_PREFIX) == spec){
        std::cerr << spec << std::endl;
        // COM�|�[�g
        // COM_name[:baudrate] format is acceptable.
        char *baudrate_spec((char *)std::strchr(spec, ':'));
        if(baudrate_spec){
          *baudrate_spec = '\0';
          baudrate_spec++;
        }
        if(iostream_pool.find(spec) == iostream_pool.end()){
          ComportStream *com_out = new ComportStream(spec);
          if(baudrate_spec){set_baudrate(*com_out, baudrate_spec);}
          iostream_pool[spec] = com_out;
          return *com_out;
        }else{
          return *(iostream_pool[spec]);
        }
      }
    }
    
    std::cerr << spec;
    std::fstream *fout(new std::fstream(spec, std::ios::out | std::ios::binary));
    std::cerr << std::endl;
    iostream_pool[spec] = fout;
    return *fout;
  }
  
  std::ostream &out() const {return *_out;}

  /**
   * @param spec check target
   * @param key_head pointer to key head pointer to be stored, only available when key found.
   * @return if key found, return key length, otherwise zero.
   */
  static unsigned int get_key(const char *spec, const char **key_head){
    *key_head = NULL;
    if(std::strstr(spec, "--") != spec){return 0;}
    unsigned int offset(2);
    *key_head = &spec[2];
    while((spec[offset] != '\0') && (spec[offset] != '=')){
      offset++;
    }
    return offset - 2;
  }

  static const char *get_value(
      const char *spec,
      const unsigned int &key_length = 0, const bool &accept_no_value = true){
    unsigned int offset(key_length);
    if(key_length == 0){ // check key length
      const char *key_head;
      if((offset = get_key(spec, &key_head)) == 0){
        return NULL;
      }
    }
    offset += 2; // move pointer to position followed by "--(key)"
    if((spec[offset] != '\0') && (spec[offset] == '=')){
      return spec + offset + 1;
    }else{
      return accept_no_value ? "true" : NULL;
    }
  }

  static const char *get_value(
      const char *spec, const char *key, const bool &accept_no_value = true){
    const char *key_head;
    unsigned int key_length(get_key(spec, &key_head));
    if(key_length == 0){return NULL;}
    if(std::strncmp(key, key_head, key_length) != 0){ // check same key?
      return NULL;
    }
    return get_value(spec, key_length, accept_no_value);
  }

  static bool is_true(const char *value){
    return (std::strcmp(value, "on") == 0) || (std::strcmp(value, "true") == 0);
  }

  /**
   * Check spec
   * 
   * @param spec
   * @return (bool) True when interpreted, otherwise false.
   */
  virtual bool check_spec(const char *spec){
    using std::cerr;
    using std::endl;
    
    const char *key;
    const unsigned int key_length(get_key(spec, &key));
    if(key_length == 0){return false;}

    bool key_checked(false);

#define CHECK_KEY(name) \
  (key_checked || \
    (key_checked = (std::strncmp(key, #name, key_length) == 0)))
#define CHECK_ALIAS(name) CHECK_KEY(name)
#define CHECK_OPTION(name, novalue, operation, disp) { \
  if(CHECK_KEY(name)){ \
    const char *value(get_value(spec, key_length, novalue)); \
    if(!value){return false;} \
    {operation;} \
    std::cerr.write(key, key_length) << ": " << disp << std::endl; \
    return true; \
  } \
}
#define CHECK_OPTION_BOOL(target) \
CHECK_OPTION(target, true, target = is_true(value), (target ? "on" : "off"));

#define CHECK_GPSTIME(prefix) \
if(key_checked){ \
  const char *value(get_value(spec, key_length, false)); \
  if(!value){return false;} \
  int dummy_i; \
  double dummy_d; \
  if(std::sscanf(value, "%i:%lf", &dummy_i, &dummy_d) == 2){ \
    prefix ## _gpstime = dummy_d; \
    prefix ## _gpswn = dummy_i; \
    std::cerr.write(key, key_length) << ": " \
        << prefix ## _gpswn << ":" \
        << prefix ## _gpstime << std::endl; \
  }else{ \
    prefix ## _gpstime = atof(value); \
    std::cerr.write(key, key_length) << ": " << prefix ## _gpstime << std::endl; \
  } \
  return true; \
}

    CHECK_ALIAS(start-gpst);
    CHECK_KEY(start_gpst);
    CHECK_GPSTIME(start);

    CHECK_ALIAS(end-gpst);
    CHECK_KEY(end_gpst);
    CHECK_GPSTIME(end);
#undef CHECK_GPSTIME

    CHECK_ALIAS(start-gpswn);
    CHECK_OPTION(start_gpswn, false,
        start_gpswn = std::atof(value),
        start_gpswn);

    CHECK_ALIAS(end-gpswn);
    CHECK_OPTION(end_gpswn, false,
        end_gpswn = std::atoi(value),
        end_gpswn);

    CHECK_ALIAS(dump-update);
    CHECK_OPTION_BOOL(dump_update);

    CHECK_ALIAS(dump-correct);
    CHECK_OPTION_BOOL(dump_correct);

    CHECK_ALIAS(init-attitude-deg);
    if(CHECK_KEY(init_attitude_deg)){
      const char *value(get_value(spec, key_length, false));
      if(!value){return false;}
      int converted(std::sscanf(value, "%lf,%lf,%lf",
        &init_attitude_deg[0], &init_attitude_deg[1], &init_attitude_deg[2]));
      has_initial_attitude = true;
      std::cerr.write(key, key_length) << " (yaw, pitch, roll) (args:"
          << converted << "): "
          << init_attitude_deg[0] << ", "
          << init_attitude_deg[1] << ", "
          << init_attitude_deg[2] << std::endl;
      return true;
    }

    CHECK_ALIAS(init-yaw-deg);
    CHECK_OPTION(init_yaw_deg, false,
        init_attitude_deg[0] = atof(value),
        init_attitude_deg[0] << " [deg]");

    CHECK_OPTION_BOOL(est_bias);
    
    CHECK_OPTION_BOOL(use_udkf);
    
    CHECK_OPTION_BOOL(use_magnet);

    CHECK_OPTION(mag_heading_accuracy_deg, false,
        mag_heading_accuracy_deg = std::atof(value),
        mag_heading_accuracy_deg << " [deg]");

    CHECK_OPTION(yaw_correct_with_mag_when_speed_less_than_ms, false,
        yaw_correct_with_mag_when_speed_less_than_ms = std::atof(value),
        yaw_correct_with_mag_when_speed_less_than_ms << " [m/s]");

    CHECK_ALIAS(out_N_packet);
    CHECK_OPTION_BOOL(out_is_N_packet);
    
    CHECK_OPTION_BOOL(reduce_1pps_sync_error);
    
    if(CHECK_KEY(out)){
      const char *value(get_value(spec, key_length, false));
      if(value){
        cerr << "out: ";
        _out = &(spec2ostream(value));
        return true;
      }
    }
    
    CHECK_OPTION_BOOL(in_sylphide);

    CHECK_OPTION_BOOL(out_sylphide);
#undef CHECK_OPTION_BOOL
#undef CHECK_OPTION
    return false;
  }
};

class NAVData {
  public:
    virtual float_sylph_t longitude() const = 0;
    virtual float_sylph_t latitude() const = 0;
    virtual float_sylph_t height() const = 0;
    virtual float_sylph_t v_north() const = 0;
    virtual float_sylph_t v_east() const = 0;
    virtual float_sylph_t v_down() const = 0;
    virtual float_sylph_t heading() const = 0;
    virtual float_sylph_t euler_phi() const = 0;
    virtual float_sylph_t euler_theta() const = 0;
    virtual float_sylph_t euler_psi() const = 0;
    virtual float_sylph_t azimuth() const = 0;
    
    /**
     * Print label
     * 
     */
    virtual void label(std::ostream &out = std::cout) const {
      out << "longitude" << ", "
           << "latitude" << ", "
           << "height" << ", "
           << "v_north" << ", "
           << "v_east" << ", "
           << "v_down" << ", "
           << "Yaw(psi)" << ", "
           << "Pitch(theta)" << ", "
           << "Roll(phi)" << ", "
           << "Azimuth(alpha)" << ", ";
    }
    
  protected:
    /**
     * Print current state
     * 
     * @param itow Time stamp
     */
    virtual void dump(std::ostream &out) const {
      out << rad2deg(longitude()) << ", "
           << rad2deg(latitude()) << ", "
           << height() << ", "
           << v_north() << ", "
           << v_east() << ", "
           << v_down() << ", "
           << rad2deg(heading()) << ", "      // yaw   <- q_{g}^{b}
           << rad2deg(euler_theta()) << ", "  // pitch <- q_{n}^{b}
           << rad2deg(euler_phi()) << ", "    // roll  <- q_{n}^{b}
           << rad2deg(azimuth()) << ", ";     // azimuth)
    }
    
  public:
    /**
     * Print current state
     * 
     * @param itow Time stamp
     */
    friend std::ostream &operator<<(std::ostream &out, const NAVData &nav){
      nav.dump(out);
      return out;
    }
    
    /**
     * Make N0 packet
     * 
     */
    void encode_N0(
        const float_sylph_t &itow,
        char buf[32]) const {
      typedef unsigned int v_u32_t;
      typedef int v_s32_t;
      typedef short v_s16_t;
      
      v_u32_t t(itow * 1000);
      v_s32_t lat(rad2deg(latitude()) * 1E7), 
          lng(rad2deg(longitude()) * 1E7), 
          h(height() * 1E4);
      v_s16_t v_n(v_north() * 1E2), 
          v_e(v_east() * 1E2), 
          v_d(v_down() * 1E2);
      v_s16_t psi(rad2deg(heading()) * 1E2), 
          theta(rad2deg(euler_theta()) * 1E2), 
          phi(rad2deg(euler_phi()) * 1E2);
      buf[0] = 'N';
      buf[1] = '\0';
      buf[2] = '\0';
      buf[3] = '\0';
      *(v_u32_t *)(&buf[4]) = le_char4_2_num<v_u32_t>(*(const char *)&t);
      *(v_s32_t *)(&buf[8]) = le_char4_2_num<v_s32_t>(*(const char *)&lat);
      *(v_s32_t *)(&buf[12]) = le_char4_2_num<v_s32_t>(*(const char *)&lng);
      *(v_s32_t *)(&buf[16]) = le_char4_2_num<v_s32_t>(*(const char *)&h);
      *(v_s16_t *)(&buf[20]) = le_char4_2_num<v_s16_t>(*(const char *)&v_n);
      *(v_s16_t *)(&buf[22]) = le_char4_2_num<v_s16_t>(*(const char *)&v_e);
      *(v_s16_t *)(&buf[24]) = le_char4_2_num<v_s16_t>(*(const char *)&v_d);
      *(v_s16_t *)(&buf[26]) = le_char4_2_num<v_s16_t>(*(const char *)&psi);
      *(v_s16_t *)(&buf[28]) = le_char4_2_num<v_s16_t>(*(const char *)&theta);
      *(v_s16_t *)(&buf[30]) = le_char4_2_num<v_s16_t>(*(const char *)&phi);
    }
};

#endif
