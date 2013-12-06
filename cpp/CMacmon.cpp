#include "CMacmon.h"

using namespace std;

CMacmon::CMacmon(string iface, UpdateCallbackType callback, void *obj, int update_freq)
{
  _updateCallBack = callback;
  _interface = iface;
  _updateFrq = update_freq;
  _uid = 0;
  _obj = obj;
}

void CMacmon::setUserId(uid_t uid)
{
  CMacmon::_uid = uid;
}

int CMacmon::start(string &errStr)
{
  time_t lastUpdate;
  int i;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t* descr;
  const u_char *packet;
  struct pcap_pkthdr *hdr;     // pcap.h 
  struct ether_header *eptr;   // net/ethernet.h 
  int retval;
  u_char *ptr;  
  string addr;
  char addrElement[8];

  descr = pcap_open_live(_interface.c_str(),BUFSIZ,1,500,errbuf);

  if(descr == NULL)
  {
      errStr = "pcap_open_live(): " + string(errbuf);
      return -1;
  }

  // Switch to less privileged user if set
  if (_uid)
    setuid(_uid);

  while (1)
  {
    lastUpdate = time(NULL);
    while ((time(NULL) - lastUpdate) < (_updateFrq))
    {
      retval = pcap_next_ex(descr, &hdr, &packet);

      if(retval == 1) // if packet received
      {
        eptr = (struct ether_header *) packet;
        ptr = eptr->ether_shost;
        i = ETHER_ADDR_LEN;
        addr = "";
        do
        {
          sprintf(addrElement, "%s%02x",(i == ETHER_ADDR_LEN) ? " " : ":",*ptr++);
          addr += addrElement;
        } while(--i>0);

        _addresses[addr] = time(NULL);
      }

    } // end while time/update
    _updateCallBack(_obj, _addresses);
    _addresses.clear();
  }

  return 0;
}

