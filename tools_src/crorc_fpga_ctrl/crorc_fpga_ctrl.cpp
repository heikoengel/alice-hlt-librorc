/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <librorc.h>
#include <getopt.h>
#include "class_crorc.hpp"

using namespace ::std;

crorc *rorc = NULL;
static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"fan", optional_argument, 0, 'f'},
    {"linkmask", optional_argument, 0, 'l'},
    {"bracketled", optional_argument, 0, 'b'},
    {"device", required_argument, 0, 'n'},
    {"channel", required_argument, 0, 'c'},
    {"board_reset", no_argument, 0, 'r'},
    {"linkspeed", optional_argument, 0, 's'},
    {"listlinkspeeds", no_argument, 0, 'S'},
    {"gtxreset", optional_argument, 0, 'R'},
    {"gtxloopback", optional_argument, 0, 'L'},
    {"gtxrxeqmix", optional_argument, 0, 'M'},
    {0, 0, 0, 0}};

void list_options() {
  int nargs = sizeof(long_options) / sizeof(option);
  for (int i = 0; i < nargs; i++) {
    cout << long_options[i].name << endl;
  }
}

typedef struct {
  uint32_t dev;
  uint32_t ch;
  bool getFanState;
  bool setFanState;
  tFanState fanStateValue;
  bool getLedState;
  bool setLedState;
  tLedState ledStateValue;
  bool getLinkmask;
  bool setLinkmask;
  uint32_t linkmaskValue;
  bool doBoardReset;
  bool getLinkspeed;
  bool setLinkspeed;
  uint32_t linkspeedValue;
  bool listLinkSpeeds;
} tRorcCmd;

int main(int argc, char *argv[]) {

  tRorcCmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.ch = 0xffffffff;

  /** Parse command line arguments **/
  if (argc > 1) {
    while (1) {
      int opt = getopt_long(argc, argv, "hf::l::b::n:c:rs::S::R::L::M::",
                            long_options, NULL);
      if (opt == -1) {
        break;
      }

      switch (opt) {
      case 'h':
        // help
        list_options();
        return 0;

      case 'f':
        // fan: on, off, auto
        if (optarg) {
          if (strcmp(optarg, "on") == 0) {
            cmd.fanStateValue = FAN_ON;
            cmd.setFanState = true;
          } else if (strcmp(optarg, "off") == 0) {
            cmd.fanStateValue = FAN_OFF;
            cmd.setFanState = true;
          } else if (strcmp(optarg, "auto") == 0) {
            cmd.fanStateValue = FAN_AUTO;
            cmd.setFanState = true;
          } else {
            cout << "Invalid argument to 'fan', allowed are 'on', 'off' and "
                    "'auto'." << endl;
            return -1;
          }
        } else {
          cmd.getFanState = true;
        }
        break;

      case 'l':
        // linkmask
        if (optarg) {
          cmd.linkmaskValue = strtol(optarg, NULL, 0);
          cmd.setLinkmask = true;
        } else {
          cmd.getLinkmask = true;
        }

      case 'b':
        // bracketled: auto, blink
        if (optarg) {
          if (strcmp(optarg, "auto") == 0) {
            cmd.ledStateValue = LED_AUTO;
            cmd.setLedState = true;
          } else if (strcmp(optarg, "blink") == 0) {
            cmd.ledStateValue = LED_BLINK;
            cmd.setLedState = true;
          } else {
            cout << "Invalid argument to 'bracketled', allowed are 'auto' and "
                    "'blink'." << endl;
            return -1;
          }
        } else {
          cmd.getLedState = true;
        }
        break;

      case 'n':
        // device
        cmd.dev = strtol(optarg, NULL, 0);
        break;

      case 'c':
        // channel
        cmd.ch = strtol(optarg, NULL, 0);
        break;

      case 'r':
        // board_reset
        cmd.doBoardReset = true;
        break;

      case 's':
        // linkspeed [specifier number]
        if(optarg) {
            cmd.linkspeedValue = strtol(optarg, NULL, 0);
            cmd.setLinkspeed = true;
        } else {
            cmd.getLinkspeed = true;
        }
        break;

      case 'S':
        // listlinkspeeds
        cmd.listLinkSpeeds = true;
        break;

      case '?':
        return -1;
      default:
        continue;
      }
    }
  } else {
    return -1;
  }

  try {
    rorc = new crorc(cmd.dev);
  } catch (...) {
    cerr << "Failed to intialize RORC0" << endl;
    abort();
  }

  if (cmd.getFanState) {
    cout << "Fan State: " << rorc->getFanState() << endl;
  } else if (cmd.setFanState) {
    rorc->setFanState(cmd.fanStateValue);
  }

  if (cmd.getLedState) {
    cout << "Bracket LED State: " << rorc->getLedState() << endl;
  } else if (cmd.setLedState) {
    rorc->setLedState(cmd.ledStateValue);
  }

  if (cmd.getLinkmask) {
    cout << "Linkmask 0x" << hex << rorc->getLinkmask() << dec << endl;
  } else if (cmd.setLinkmask) {
    rorc->setLinkmask(cmd.linkmaskValue);
  }

  if (cmd.doBoardReset) {
    rorc->doBoardReset();
  }

  if (cmd.getLinkspeed) {
    librorc::refclkopts clkopts = rorc->m_refclk->getCurrentOpts(0);
    float refclkfreq = rorc->m_refclk->getFout(clkopts);
    librorc::gtxpll_settings pll = rorc->m_gtx[0]->drpGetPllConfig();
    uint32_t linkspeed = 2 * refclkfreq * pll.n1 * pll.n2 / pll.m / pll.d;
    cout << "GTX0 Link Speed: " << linkspeed << " Mbps, RefClk: " << refclkfreq
         << " MHz" << endl;
  } else if (cmd.setLinkspeed) {
  }

  if (cmd.listLinkSpeeds) {
    int nCfgs = sizeof(librorc::gtxpll_supported_cfgs) /
                sizeof(librorc::gtxpll_settings);
    cout << "Number of supported configurations: " << nCfgs << endl;
    for (int i = 0; i < nCfgs; i++) {
      librorc::gtxpll_settings pll = librorc::gtxpll_supported_cfgs[i];
      uint32_t linkspeed = 2 * pll.refclk * pll.n1 * pll.n2 / pll.m / pll.d;
      cout << "Linkspeed: " << linkspeed << " Mbps, RefClk: " << pll.refclk
           << " MHz" << endl;
    }
    return 0;
  }

  if (rorc) {
    delete rorc;
  }
  return 0;
}
