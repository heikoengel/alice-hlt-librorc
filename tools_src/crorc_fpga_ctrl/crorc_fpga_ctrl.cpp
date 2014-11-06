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
 *
 * TODO:
 * - check GTX/DDL Domain is up before accessing
 **/

#include <librorc.h>
#include <getopt.h>
#include "class_crorc.hpp"

using namespace ::std;

void list_options(const struct option *long_options, int nargs) {
  cout << "Available arguments:" << endl;
  for (int i = 0; i < nargs; i++) {
    if (long_options[i].name) {
      cout << "--" << long_options[i].name;
    }
    switch (long_options[i].has_arg) {
    case optional_argument:
      cout << " ([value])";
      break;
    default:
      break;
    }
    cout << endl;
  }
}

void print_linkpllstate(librorc::gtxpll_settings pllcfg) {
  uint32_t linkspeed =
      2 * pllcfg.refclk * pllcfg.n1 * pllcfg.n2 / pllcfg.m / pllcfg.d;
  cout << "GTX Link Speed: " << linkspeed << " Mbps, RefClk: " << pllcfg.refclk
       << " MHz" << endl;
}

void print_gtxstate(uint32_t i, librorc::gtx *gtx) {
  cout << "GTX" << i << endl;
  cout << "\tReset        : " << gtx->getReset() << endl
       << "\tLoopback     : " << gtx->getLoopback() << endl
       << "\tTxDiffCtrl   : " << gtx->getTxDiffCtrl() << endl
       << "\tTxPreEmph    : " << gtx->getTxPreEmph() << endl
       << "\tTxPostEmph   : " << gtx->getTxPostEmph() << endl
       << "\tTxEqMix      : " << gtx->getRxEqMix() << endl
       << "\tDfeEye       : " << gtx->dfeEye() << endl;
  if (gtx->isDomainReady()) {
    cout << "\tDomain       : up" << endl;
    cout << "\tLink Up      : " << gtx->isLinkUp() << endl;
    cout << "\tDisp.Errors  : " << gtx->getDisparityErrorCount() << endl
         << "\tRealignCount : " << gtx->getRealignCount() << endl
         << "\tRX NIT Errors: " << gtx->getRxNotInTableErrorCount() << endl
         << "\tRX LOS Errors: " << gtx->getRxLossOfSignalErrorCount() << endl;
  } else {
    cout << "\tDomain       : DOWN!" << endl;
  }
}

void print_ddlstate(uint32_t i, crorc *rorc) {
  cout << "DDL" << i << " Status" << endl;
  cout << "\tReset       : " << rorc->m_ddl[i]->getReset() << endl;
  cout << "\tIF-Enable   : " << rorc->m_ddl[i]->getEnable() << endl;
  cout << "\tLink Full   : " << rorc->m_ddl[i]->linkFull() << endl;
  cout << "\tDMA Deadtime: " << rorc->m_ddl[i]->getDmaDeadtime() << endl;
  cout << "\tEventcount  : " << rorc->m_ddl[i]->getEventcount() << endl;
  if (rorc->m_diu[i] != NULL) {
    cout << "\tLink Up     : " << rorc->m_diu[i]->linkUp() << endl;
    cout << "\tDDL Deadtime: " << rorc->m_diu[i]->getDdlDeadtime() << endl;
    cout << "\tLast Command: 0x" << hex << rorc->m_diu[i]->lastDiuCommand()
         << dec << endl;
    cout << "\tLast FESTW: 0x" << hex
         << rorc->m_diu[i]->lastFrontEndStatusWord() << dec << endl;
    cout << "\tLast CTSTW: 0x" << hex
         << rorc->m_diu[i]->lastCommandTransmissionStatusWord() << dec << endl;
    cout << "\tLast DTSTW: 0x" << hex
         << rorc->m_diu[i]->lastDataTransmissionStatusWord() << dec << endl;
    cout << "\tLast IFSTW: 0x" << hex
         << rorc->m_diu[i]->lastInterfaceStatusWord() << dec << endl;
  }
  if (rorc->m_siu[i] != NULL) {
    cout << "\tDDL Deadtime: " << rorc->m_siu[i]->getDdlDeadtime() << endl;
    cout << "\tLast FECW: 0x" << hex
         << rorc->m_siu[i]->lastFrontEndCommandWord() << dec << endl;
    cout << "IFFIFOEmpty: " << rorc->m_siu[i]->isInterfaceFifoEmpty() << endl;
    cout << "IFFIFOFull : " << rorc->m_siu[i]->isInterfaceFifoFull() << endl;
    cout << "SourceEmpty: " << rorc->m_siu[i]->isSourceEmpty() << endl;
  }
}

typedef struct {
  bool set;
  bool get;
  uint32_t value;
} tControlSet;

typedef struct {
  uint32_t dev;
  uint32_t ch;
  int boardReset;
  bool listLinkSpeeds;
  int gtxClearCounters;
  int gtxStatus;
  int ddlClearCounters;
  int refclkReset;
  int ddlStatus;
  int diuInitRemoteDiu;
  int diuInitRemoteSiu;
  tControlSet fan;
  tControlSet led;
  tControlSet linkmask;
  tControlSet linkspeed;
  tControlSet gtxReset;
  tControlSet gtxLoopback;
  tControlSet gtxRxeqmix;
  tControlSet gtxTxdiffctrl;
  tControlSet gtxTxpreemph;
  tControlSet gtxTxpostemph;
  tControlSet ddlReset;
  tControlSet diuSendCommand;
} tRorcCmd;

tControlSet evalParam(char *param) {
  tControlSet cs = {false, false, 0};
  if (param) {
    cs.value = strtol(param, NULL, 0);
    cs.set = true;
  } else {
    cs.get = true;
  }
  return cs;
}

int main(int argc, char *argv[]) {

  crorc *rorc = NULL;
  tRorcCmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.ch = LIBRORC_CH_UNDEF;

  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"fan", optional_argument, 0, 'f'},
      {"linkmask", optional_argument, 0, 'l'},
      {"bracketled", optional_argument, 0, 'b'},
      {"device", required_argument, 0, 'n'},
      {"channel", required_argument, 0, 'c'},
      {"boardreset", no_argument, &(cmd.boardReset), 1},
      {"linkspeed", optional_argument, 0, 's'},
      {"listlinkspeeds", no_argument, 0, 'S'},
      {"gtxreset", optional_argument, 0, 'R'},
      {"gtxloopback", optional_argument, 0, 'L'},
      {"gtxrxeqmix", optional_argument, 0, 'M'},
      {"gtxtxdiffctrl", optional_argument, 0, 'D'},
      {"gtxtxpreemph", optional_argument, 0, 'P'},
      {"gtxtxpostemph", optional_argument, 0, 'O'},
      {"gtxclearcounters", no_argument, &(cmd.gtxClearCounters), 1},
      {"gtxstatus", no_argument, &(cmd.gtxStatus), 1},
      {"ddlreset", optional_argument, 0, 'd'},
      {"ddlclearcounters", no_argument, &(cmd.ddlClearCounters), 1},
      {"ddlstatus", no_argument, &(cmd.ddlStatus), 1},
      {"diuinitremotesiu", no_argument, &(cmd.diuInitRemoteSiu), 1},
      {"diuinitremotediu", no_argument, &(cmd.diuInitRemoteDiu), 1},
      {"diusendcmd", optional_argument, 0, 'C'},
      {"refclkreset", no_argument, &(cmd.refclkReset), 1},
      {0, 0, 0, 0}};
  int nargs = sizeof(long_options) / sizeof(option);

  /** Parse command line arguments **/
  if (argc > 1) {
    while (1) {
      int opt =
          getopt_long(argc, argv, "hf::l::b::n:c:s::S::R::L::M::D::P::O::d::C::",
                      long_options, NULL);
      if (opt == -1) {
        break;
      }

      switch (opt) {
      case 'h':
        // help
        list_options(long_options, nargs);
        return 0;
        break;

      case 'f':
        // fan: on, off, auto
        if (optarg) {
          if (strcmp(optarg, "on") == 0) {
            cmd.fan.value = FAN_ON;
            cmd.fan.set = true;
          } else if (strcmp(optarg, "off") == 0) {
            cmd.fan.value = FAN_OFF;
            cmd.fan.set = true;
          } else if (strcmp(optarg, "auto") == 0) {
            cmd.fan.value = FAN_AUTO;
            cmd.fan.set = true;
          } else {
            cout << "Invalid argument to 'fan', allowed are 'on', 'off' and "
                    "'auto'." << endl;
            return -1;
          }
        } else {
          cmd.fan.get = true;
        }
        break;

      case 'l':
        // linkmask
        cmd.linkmask = evalParam(optarg);
        break;

      case 'b':
        // bracketled: auto, blink
        if (optarg) {
          if (strcmp(optarg, "auto") == 0) {
            cmd.led.value = LED_AUTO;
            cmd.led.set = true;
          } else if (strcmp(optarg, "blink") == 0) {
            cmd.led.value = LED_BLINK;
            cmd.led.set = true;
          } else {
            cout << "Invalid argument to 'bracketled', allowed are 'auto' and "
                    "'blink'." << endl;
            return -1;
          }
        } else {
          cmd.led.get = true;
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

      case 's':
        // linkspeed [specifier number]
        cmd.linkspeed = evalParam(optarg);
        break;

      case 'S':
        // listlinkspeeds
        cmd.listLinkSpeeds = true;
        break;

      case 'R':
        // gtxreset
        cmd.gtxReset = evalParam(optarg);
        break;

      case 'L':
        // gtxloopback
        cmd.gtxLoopback = evalParam(optarg);
        break;

      case 'M':
        // gtxrxeqmix
        cmd.gtxRxeqmix = evalParam(optarg);
        break;

      case 'D':
        // gtxtxdiffctrl
        cmd.gtxTxdiffctrl = evalParam(optarg);
        break;

      case 'P':
        // gtxtxpreemph
        cmd.gtxTxpreemph = evalParam(optarg);
        break;

      case 'O':
        // gtxtxpostemph
        cmd.gtxTxpostemph = evalParam(optarg);
        break;

      case 'd':
        // ddlreset
        cmd.ddlReset = evalParam(optarg);
        break;

      case 'C':
        // ddlreset
        cmd.diuSendCommand = evalParam(optarg);
        break;

      case '?':
        return -1;
      default:
        continue;
      }
    }
  } else {
    list_options(long_options, nargs);
    return -1;
  }

  uint32_t nPllCfgs =
      sizeof(librorc::gtxpll_supported_cfgs) / sizeof(librorc::gtxpll_settings);

  if (cmd.listLinkSpeeds) {
    cout << "Number of supported configurations: " << nPllCfgs << endl;
    for (uint32_t i = 0; i < nPllCfgs; i++) {
      librorc::gtxpll_settings pll = librorc::gtxpll_supported_cfgs[i];
      cout << i << ") ";
      print_linkpllstate(pll);
    }
    return 0;
  }

  try {
    rorc = new crorc(cmd.dev);
  } catch (...) {
    cerr << "Failed to intialize RORC0" << endl;
    abort();
  }

  if (cmd.fan.get) {
    cout << "Fan State: " << rorc->getFanState() << endl;
  } else if (cmd.fan.set) {
    rorc->setFanState(cmd.fan.value);
  }

  if (cmd.led.get) {
    cout << "Bracket LED State: " << rorc->getLedState() << endl;
  } else if (cmd.led.set) {
    rorc->setLedState(cmd.led.value);
  }

  if (cmd.linkmask.get) {
    cout << "Linkmask 0x" << hex << rorc->getLinkmask() << dec << endl;
  } else if (cmd.linkmask.set) {
    rorc->setLinkmask(cmd.linkmask.value);
  }

  if (cmd.boardReset) {
    rorc->doBoardReset();
    // TODO
  }

  uint32_t ch_start, ch_end;
  if (cmd.ch == LIBRORC_CH_UNDEF) {
    ch_start = 0;
    ch_end = rorc->m_nchannels - 1;
  } else {
    ch_start = cmd.ch;
    ch_end = cmd.ch;
  }

  if (cmd.linkspeed.get) {
    librorc::refclkopts clkopts = rorc->m_refclk->getCurrentOpts(0);
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      librorc::gtxpll_settings pllsts = rorc->m_gtx[i]->drpGetPllConfig();
      pllsts.refclk = rorc->m_refclk->getFout(clkopts);
      cout << "Ch" << i << " ";
      print_linkpllstate(pllsts);
    }
  } else if (cmd.linkspeed.set) {
    if (cmd.linkspeed.value > nPllCfgs) {
      cout << "ERROR: invalid PLL config selected." << endl;
      return -1;
    }
    librorc::gtxpll_settings pllcfg =
        librorc::gtxpll_supported_cfgs[cmd.linkspeed.value];

    // set QSFPs and GTX to reset
    rorc->setAllQsfpReset(1);
    rorc->setAllGtxReset(1);

    // reconfigure reference clock
    rorc->m_refclk->reset();
    if (pllcfg.refclk != LIBRORC_REFCLK_DEFAULT_FOUT) {
      librorc::refclkopts refclkopts =
          rorc->m_refclk->getCurrentOpts(LIBRORC_REFCLK_DEFAULT_FOUT);
      librorc::refclkopts new_refclkopts =
          rorc->m_refclk->calcNewOpts(pllcfg.refclk, refclkopts.fxtal);
      rorc->m_refclk->writeOptsToDevice(new_refclkopts);
    }

    // reconfigure GTX
    rorc->configAllGtxPlls(pllcfg);

    // release GTX and QSFP resets
    rorc->setAllGtxReset(0);
    rorc->setAllQsfpReset(0);

    print_linkpllstate(pllcfg);
  }

  if (cmd.listLinkSpeeds) {
    cout << "Number of supported configurations: " << nPllCfgs << endl;
    for (uint32_t i = 0; i < nPllCfgs; i++) {
      librorc::gtxpll_settings pll = librorc::gtxpll_supported_cfgs[i];
      cout << i << ") ";
      print_linkpllstate(pll);
    }
  }

  if (cmd.gtxReset.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i << " GTX Reset: " << rorc->m_gtx[i]->getReset() << endl;
    }
  } else if (cmd.gtxReset.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->setReset(cmd.gtxReset.value);
    }
  }

  if (cmd.gtxLoopback.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i << " GTX Loopback: " << rorc->m_gtx[i]->getLoopback();
    }
  } else if (cmd.gtxLoopback.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->setLoopback(cmd.gtxLoopback.value);
    }
  }

  if (cmd.gtxRxeqmix.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i << " GTX RxEqMix: " << rorc->m_gtx[i]->getRxEqMix();
    }
  } else if (cmd.gtxRxeqmix.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->setRxEqMix(cmd.gtxRxeqmix.value);
    }
  }

  if (cmd.gtxTxpreemph.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i << " GTX TxPreEmph: " << rorc->m_gtx[i]->getTxPreEmph();
    }
  } else if (cmd.gtxTxpreemph.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->setTxPreEmph(cmd.gtxTxpreemph.value);
    }
  }

  if (cmd.gtxTxpostemph.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i
           << " GTX TxPostEmph: " << rorc->m_gtx[i]->getTxPostEmph();
    }
  } else if (cmd.gtxTxpostemph.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->setTxPostEmph(cmd.gtxTxpostemph.value);
    }
  }

  if (cmd.gtxClearCounters) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_gtx[i]->clearErrorCounters();
    }
  }

  if (cmd.gtxStatus){
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      print_gtxstate(i, rorc->m_gtx[i]);
    }
  }

  if (cmd.ddlReset.get) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      cout << "Ch" << i << "DDL Reset: " << rorc->m_ddl[i]->getReset() << endl;
    }
  } else if (cmd.ddlReset.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      rorc->m_ddl[i]->setReset(cmd.ddlReset.value);
    }
  }

  if (cmd.ddlClearCounters) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      if (rorc->m_diu[i] != NULL) {
        rorc->m_diu[i]->clearAllLastStatusWords();
        rorc->m_diu[i]->clearDdlDeadtime();
      } else if (rorc->m_siu[i] != NULL) {
        rorc->m_siu[i]->clearLastFrontEndCommandWord();
        rorc->m_siu[i]->clearDdlDeadtime();
      }
      rorc->m_ddl[i]->clearEventcount();
      rorc->m_ddl[i]->clearDmaDeadtime();
    }
  }

  if (cmd.ddlStatus) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      print_ddlstate(i, rorc);
    }
  }

  if (cmd.diuInitRemoteDiu) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      if (rorc->m_diu[i] != NULL) {
        if (rorc->m_diu[i]->prepareForDiuData()) {
          cout << "DIU" << i << " Failed to init remote DIU" << endl;
        }
      } else {
        cout << "Link" << i << " has no local DIU, cannot init remote DIU"
             << endl;
      }
    }
  }

  if (cmd.diuInitRemoteSiu) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      if (rorc->m_diu[i] != NULL) {
        if (rorc->m_diu[i]->prepareForSiuData()) {
          cout << "DIU" << i << " Failed to init remote SIU" << endl;
        }
      } else {
        cout << "Link" << i << " has no local DIU, cannot init remote SIU"
             << endl;
      }
    }
  }

  // note: cmd.diuSendCommand.get is not handled here
  if (cmd.diuSendCommand.set) {
    for (uint32_t i = ch_start; i <= ch_end; i++) {
      if (rorc->m_diu[i] != NULL) {
        rorc->m_diu[i]->sendCommand(cmd.diuSendCommand.value);
      } else {
        cout << "Link" << i << " has no local DIU, cannot send command" << endl;
      }
    }
  }

  if (cmd.refclkReset) {
    rorc->m_refclk->reset();
  }

  if (rorc) {
    delete rorc;
  }
  return 0;
}
