//
//  UPnPPortMap.hpp
//
//
//

#ifndef UPnPPortMap_hpp
#define UPnPPortMap_hpp

#include <stdio.h>
#include <string>
#include <list>



class UPnPPortMap
{
public:
    struct PortMapInfo
    {
      std::string localIP;  //要进行端口映射的IP
      std::string localPort;    //IP对应的端口号
      std::string remotePort;   //路由器上端口号(大于1024）
      std::string protocolType; //进行映射的协议类型,只能为UDP或者TCP
    };

public:
    /*
     *初始化
     *@param none
     *@param none
     *@return int
     *@note 调用接下来的三个函数前必须先调用init，因为初始化时有socket操作，因此可能会有一些延迟，尽量不要在构造里调用,
     *如果返回值不为0，说明初始化失败，初始化失败时不能调用接下来的三个函数
     */
    int init();
    /*
     *增加一个端口映射
     *@param (PortMapInfo)info
     *@return none
     *note 填充PortMapInfo类型来添加一个端口映射
     PortMapInfo中remotePort如果已经被添加，调用函数后会覆盖之前的,
     *remotePort必须大于1024
     */
    int addPortMapping(PortMapInfo info);
    /*
     *删除一个端口映射
     *@param (PortMapInfo)info
     *@return none
     *note 和addPortMapping类似
     */
    int deletePortMapping(PortMapInfo info);
    /*
     *获取当前已经存在的端口映射
     *@param none
     *@return std::list<PortMapInfo>
     */
    std::list<PortMapInfo> getPortMappingList();

    /*
     *获取路由器公网IP
     *@param none
     *@return std::string
     */
    std::string getExternalIPAddress();
private:
    std::string getElementValueByName(std::string& xmlStr,std::string name);
    void fillPortMapInfoFromXMLString(std::string& xmlStr,PortMapInfo& info);
    std::string httpGet(std::string ip,std::string port,std::string path);
    PortMapInfo getGenericPortMappingEntry(int index,bool& success);
    int GetRouterInfo();
private:
    std::string routerIP;
    std::string routerPort;
    std::string routerControlUrl;
};


#endif /* UPnPPortMap_hpp */
