#include "UPnPPortMap.h"
#include <iostream>
int main()
{
  UPnPPortMap m;
  m.init();
  std::list<UPnPPortMap::PortMapInfo> infos=m.getPortMappingList();
  UPnPPortMap::PortMapInfo i;
  i.localIP="10.0.0.83";
  i.remotePort="55442";
  i.localPort="12353";
  i.protocolType="TCP";
  int r=m.addPortMapping(i);
  std::cout<<"r : "<<r<<std::endl;
  for(std::list<UPnPPortMap::PortMapInfo>::iterator it=infos.begin();it!=infos.end();it++)
  {
    std::cout<<it->remotePort<<" ------> "<<it->localIP<<":"<<it->localPort<<" "<<it->protocolType<<std::endl;
  }

  std::string externalIPAddressStr=m.getExternalIPAddress();
  std::cout<<"ip : "<<externalIPAddressStr<<std::endl;
  return 0;
}
