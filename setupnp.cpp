#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>


#define ActionAddPortMapping "urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping"
#define ActionGetGenericPortMappingEntry "urn:schemas-upnp-org:service:WANIPConnection:1#GetGenericPortMappingEntry"
#define ActionDeletePortMapping "urn:schemas-upnp-org:service:WANIPConnection:1#DeletePortMapping"
#define ActionGetExternalIPAddress "urn:schemas-upnp-org:service:WANIPConnection:1#GetExternalIPAddress"

#define MS_ST "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
#define MS_MAX "30"
char MSearchRequest[]="M-SEARCH * HTTP/1.1\r\n" \
"HOST: 239.255.255.250:1900\r\n" \
"MAN: \"ssdp:discover\"\r\n" \
"MX: " MS_MAX "\r\n" \
"ST: " MS_ST "\r\n\r\n";



#define HOSTIP "[HostIP]"
#define CONTENTLENGTH "[ContentLength]"
#define URL "[URL]"
#define ACTION "[Action]"

//addPortMapping argument macro
#define PM_EXTERNALPORT "[PM_ExternalPort]"
#define PM_PROTONAME "[PM_ProtocolName]"
#define PM_LOCALPORT "[PM_LocalPort]"
#define PM_LOCALIP "[PM_LocalIP]"
#define PM_ENABLE "[PM_Enable]"
#define PM_DESCRIPTION "[PM_DESCRIPTION]"
#define PM_DURATION "0"

//getGenericPortMappingEntry argument macro
#define PM_INDEX "[PM_Index]"

char WANNIPConnectionHeaderFmtStr[]="POST " URL " HTTP/1.1\r\n" \
"Host: " HOSTIP "\r\n" \
"User-Agent: UPnP/1.1\r\n" \
"Content-Length: " CONTENTLENGTH "\r\n" \
"Content-Type: text/xml\r\n" \
"SOAPAction: \"" ACTION "\"\r\n" \
"Connection: Close\r\n" \
"Cache-Control: no-cache\r\n" \
"Pragma: no-cache\r\n\r\n";

char addPortMappingBodyFmtStr[]="<?xml version=\"1.0\"?>" \
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
"<s:Body>" \
"<u:AddPortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">" \
"<NewRemoteHost>" \
"</NewRemoteHost>" \
"<NewExternalPort>" \
PM_EXTERNALPORT  \
"</NewExternalPort>" \
"<NewProtocol>" \
PM_PROTONAME  \
"</NewProtocol>" \
"<NewInternalPort>" \
PM_LOCALPORT  \
"</NewInternalPort>" \
"<NewInternalClient>" \
PM_LOCALIP  \
"</NewInternalClient>" \
"<NewEnabled>" \
PM_ENABLE  \
"</NewEnabled>" \
"<NewLeaseDuration>" \
PM_DURATION  \
"</NewLeaseDuration>" \
"<NewPortMappingDescription>" \
PM_DESCRIPTION  \
"</NewPortMappingDescription>" \
"</u:AddPortMapping></s:Body></s:Envelope>";


char getGenericPortMappingEntryBodyFmtStr[]="<?xml version=\"1.0\"?>" \
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
"<s:Body>" \
"<u:GetGenericPortMappingEntry xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">" \
"<NewPortMappingIndex>" \
PM_INDEX \
"</NewPortMappingIndex>" \
"</u:GetGenericPortMappingEntry></s:Body></s:Envelope>";

char deletePortMappingBodyFmtStr[]="<?xml version=\"1.0\"?>" \
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
"<s:Body>" \
"<u:DeletePortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">" \
"<NewRemoteHost>" \
"</NewRemoteHost>" \
"<NewExternalPort>" \
PM_EXTERNALPORT  \
"</NewExternalPort>" \
"<NewProtocol>" \
PM_PROTONAME  \
"</NewProtocol>" \
"</u:DeletePortMapping></s:Body></s:Envelope>";

char getExternalIPAddressFmtStr[]="<?xml version=\"1.0\"?>" \
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
"<s:Body>" \
"<u:GetExternalIPAddress xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">" \
"</u:GetExternalIPAddress></s:Body></s:Envelope>";


struct RouterInfo
{
  std::string ip;
  std::string port;
  std::string ctlurl;
};

struct PortMapInfo
{
  std::string externPort;
  std::string localPort;
  std::string localIP;
  std::string protoType;
  std::string newEnable;
  std::string description;
  bool isValid;
  PortMapInfo():isValid(false){}
};

//element without attributes
//ie.
//<Element1>
//this is my value
//</Element1>
std::string getElementValueByName(std::string& xmlStr,std::string name)
{
  std::string elementStartName="<"+name+">";
  std::string elementEndName="</"+name+">";
  int elementStart=xmlStr.find(elementStartName)+elementStartName.size();
  int elementEnd=xmlStr.find(elementEndName,elementStart);
  std::string value=xmlStr.substr(elementStart,elementEnd-elementStart);
  return value; 
}

void fillPortMapInfoFromString(std::string& xmlStr,PortMapInfo& info)
{
  info.externPort=getElementValueByName(xmlStr,"NewExternalPort");
  info.localPort=getElementValueByName(xmlStr,"NewInternalPort");
  info.localIP=getElementValueByName(xmlStr,"NewInternalClient");
  info.protoType=getElementValueByName(xmlStr,"NewProtocol");
  info.newEnable=getElementValueByName(xmlStr,"NewEnabled");
  info.description=getElementValueByName(xmlStr,"NewPortMappingDescription");
}

std::string httpGet(std::string ip,std::string port,std::string path)
{
  int socketGet=socket(AF_INET,SOCK_STREAM,0);
  struct sockaddr_in getAddr;
  getAddr.sin_family=PF_INET;
  getAddr.sin_port=htons(atoi(port.data()));
  getAddr.sin_addr.s_addr=inet_addr(ip.data());
  connect(socketGet,(const struct sockaddr*)&getAddr,sizeof(getAddr));
  std::string request="GET [PATH] HTTP/1.1\r\n" \
                       "HOST: [HOST]\r\n\r\n";
  request.replace(request.find("[PATH]"),6,path);
  request.replace(request.find("[HOST]"),6,ip+":"+port);
  sendto(socketGet,request.data(),request.size(),0,NULL,0);
  //std::cout<<"request : "<<request<<std::endl;
  std::string getContent;
  while(1)
  {
    char buff[2048];
    int ret=recvfrom(socketGet,buff,sizeof(buff),0,NULL,NULL);
    if(ret<=0)
      break;
    getContent.append(buff,ret);
  }
  close(socketGet);
  return getContent;
}



int addPortMapping(std::string host,std::string hostport,std::string ctlurl,PortMapInfo info)
{
  int ret=0;
  int socketHandle=socket(PF_INET,SOCK_STREAM,0);

  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  //addr.sin_len=sizeof(struct sockaddr_in);
  addr.sin_port=htons(atoi(hostport.data()));
  addr.sin_addr.s_addr=inet_addr(host.data());
  addr.sin_family=AF_INET;
  
  std::string header=WANNIPConnectionHeaderFmtStr;
  std::string body=addPortMappingBodyFmtStr;

  header.replace(header.find(URL),sizeof(URL)-1,ctlurl);
  header.replace(header.find(HOSTIP),sizeof(HOSTIP)-1,host+":"+hostport);
  header.replace(header.find(ACTION),sizeof(ACTION)-1,ActionAddPortMapping);
  body.replace(body.find(PM_EXTERNALPORT),sizeof(PM_EXTERNALPORT)-1,info.externPort);
  body.replace(body.find(PM_PROTONAME),sizeof(PM_PROTONAME)-1,info.protoType);
  body.replace(body.find(PM_LOCALPORT),sizeof(PM_LOCALPORT)-1,info.localPort);
  body.replace(body.find(PM_LOCALIP),sizeof(PM_LOCALIP)-1,info.localIP);
  body.replace(body.find(PM_ENABLE),sizeof(PM_ENABLE)-1,info.newEnable);
  body.replace(body.find(PM_DESCRIPTION),sizeof(PM_DESCRIPTION)-1,info.description);
  int contentLength=body.size();
  char strLength[10];
  sprintf(strLength,"%d",contentLength);
  header.replace(header.find(CONTENTLENGTH),sizeof(CONTENTLENGTH)-1,strLength);
  
  std::string request=header+body;
  std::cout<<"request : " <<request<<std::endl;


  ret=connect(socketHandle,(struct sockaddr*)&addr,sizeof(addr));

  ret=send(socketHandle,request.data(),request.size(),0);
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(errno)<<std::endl;
  }

  char buff[2048];
  memset(buff,0,2048);
  ret=recv(socketHandle,buff,2048,0);
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(errno)<<std::endl;
  }
  close(socketHandle);
  return ret;
}


PortMapInfo getGenericPortMappingEntry(std::string host,std::string hostport,std::string ctlurl,int index)
{
  PortMapInfo info;
  int ret=0;
  int socketHandle=socket(PF_INET,SOCK_STREAM,0);
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  //addr.sin_len=sizeof(struct sockaddr_in);
  addr.sin_port=htons(atoi(hostport.data()));
  addr.sin_addr.s_addr=inet_addr(host.data());
  addr.sin_family=AF_INET;
  
  std::string header=WANNIPConnectionHeaderFmtStr;
  std::string body=getGenericPortMappingEntryBodyFmtStr;

  header.replace(header.find(URL),sizeof(URL)-1,ctlurl);
  header.replace(header.find(HOSTIP),sizeof(HOSTIP)-1,host+":"+hostport);
  header.replace(header.find(ACTION),sizeof(ACTION)-1,ActionGetGenericPortMappingEntry);
  char indexStr[20];
  memset(indexStr,0,20);
  sprintf(indexStr,"%d",index);
  body.replace(body.find(PM_INDEX),sizeof(PM_INDEX)-1,indexStr);
  int contentLength=body.size();
  char strLength[10];
  sprintf(strLength,"%d",contentLength);
  header.replace(header.find(CONTENTLENGTH),sizeof(CONTENTLENGTH)-1,strLength);
  
  std::string request=header+body;


  ret=connect(socketHandle,(struct sockaddr*)&addr,sizeof(addr));

  ret=send(socketHandle,request.data(),request.size(),0);
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(ret)<<std::endl;
  }

  std::string respone;
  while(1)
  {
    char buff[2048];
    memset(buff,0,2048);
    ret=recv(socketHandle,buff,2048,0);
    if(ret>0)
    {
      respone.append(buff,ret);
    }
    else
      break;
  }
  close(socketHandle);
  int responeCodeStart=respone.find(" ")+1;
  int responeCodeEnd=respone.find(" ",responeCodeStart);
  std::string responeCode=respone.substr(responeCodeStart,responeCodeEnd-responeCodeStart);
  if(responeCode=="200")
  { 
    info.isValid=true;
    fillPortMapInfoFromString(respone,info);
  }
  return info;
}

int deletePortMapping(std::string host,std::string hostport,std::string ctlurl,PortMapInfo info)
{
  int ret=0;
  int socketHandle=socket(PF_INET,SOCK_STREAM,0);

  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  //addr.sin_len=sizeof(struct sockaddr_in);
  addr.sin_port=htons(atoi(hostport.data()));
  addr.sin_addr.s_addr=inet_addr(host.data());
  addr.sin_family=AF_INET;
  
  std::string header=WANNIPConnectionHeaderFmtStr;
  std::string body=deletePortMappingBodyFmtStr;

  header.replace(header.find(URL),sizeof(URL)-1,ctlurl);
  header.replace(header.find(HOSTIP),sizeof(HOSTIP)-1,host+":"+hostport);
  header.replace(header.find(ACTION),sizeof(ACTION)-1,ActionDeletePortMapping);
  body.replace(body.find(PM_EXTERNALPORT),sizeof(PM_EXTERNALPORT)-1,info.externPort);
  body.replace(body.find(PM_PROTONAME),sizeof(PM_PROTONAME)-1,info.protoType);
  int contentLength=body.size();
  char strLength[10];
  sprintf(strLength,"%d",contentLength);
  header.replace(header.find(CONTENTLENGTH),sizeof(CONTENTLENGTH)-1,strLength);
  
  std::string request=header+body;
  std::cout<<"request : " <<request<<std::endl;

  ret=connect(socketHandle,(struct sockaddr*)&addr,sizeof(addr));

  ret=send(socketHandle,request.data(),request.size(),0);
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(errno)<<std::endl;
  }

  char buff[2048];
  memset(buff,0,2048);
  ret=recv(socketHandle,buff,2048,0);
  std::cout<<"delete : "<<buff<<std::endl;
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(errno)<<std::endl;
  }
  close(socketHandle);
  return ret;
}

std::string getExternalIPAddress(std::string host,std::string port,std::string ctlurl)
{
  std::string externalIPAddressStr;
  int ret=0;
  int socketHandle=socket(PF_INET,SOCK_STREAM,0);
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  //addr.sin_len=sizeof(struct sockaddr_in);
  addr.sin_port=htons(atoi(port.data()));
  addr.sin_addr.s_addr=inet_addr(host.data());
  addr.sin_family=AF_INET;

  std::string header=WANNIPConnectionHeaderFmtStr;
  std::string body=getExternalIPAddressFmtStr;

  header.replace(header.find(URL),sizeof(URL)-1,ctlurl);
  header.replace(header.find(HOSTIP),sizeof(HOSTIP)-1,host+":"+port);
  header.replace(header.find(ACTION),sizeof(ACTION)-1,ActionGetExternalIPAddress);
  int contentLength=body.size();
  char strLength[10];
  sprintf(strLength,"%d",contentLength);
  header.replace(header.find(CONTENTLENGTH),sizeof(CONTENTLENGTH)-1,strLength);

  std::string request=header+body;


  ret=connect(socketHandle,(struct sockaddr*)&addr,sizeof(addr));

  ret=send(socketHandle,request.data(),request.size(),0);
  std::cout<<"request : "<<request<<std::endl;
  if(ret<0)
  {
    std::cout<<"error : "<<strerror(ret)<<std::endl;
  }

  std::string respone;
  while(1)
  {
    char buff[2048];
    memset(buff,0,2048);
    ret=recv(socketHandle,buff,2048,0);
    if(ret>0)
    {
      respone.append(buff,ret);
    }
    else
      break;
  }
  close(socketHandle);
  int responeCodeStart=respone.find(" ")+1;
  int responeCodeEnd=respone.find(" ",responeCodeStart);
  std::string responeCode=respone.substr(responeCodeStart,responeCodeEnd-responeCodeStart);
  std::cout<<"respone : "<<respone<<std::endl;
  if(responeCode=="200")
  { 
    externalIPAddressStr=getElementValueByName(respone,"NewExternalIPAddress");
  }
  return externalIPAddressStr;
}


RouterInfo GetRouterInfo()
{
  RouterInfo info;
  int ret=0;
  int socketMSearch=socket(AF_INET,SOCK_DGRAM,0);
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_port=htons(1900);
  addr.sin_addr.s_addr=inet_addr("239.255.255.250");
  addr.sin_family=PF_INET;
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr=inet_addr("239.255.255.250");
  mreq.imr_interface.s_addr=INADDR_ANY;
  ret=setsockopt(socketMSearch,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return info;
  }
  char ttl=4;
  ret=setsockopt(socketMSearch,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl));
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return info;
  }

  ret=sendto(socketMSearch,MSearchRequest,sizeof(MSearchRequest)-1,0,(struct sockaddr*)&addr,sizeof(addr));
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
  }

  char MSearchRespone[2048];
  memset(MSearchRespone,0,2048);

  ret=recvfrom(socketMSearch,MSearchRespone,sizeof(MSearchRespone),0,NULL,NULL);
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return info;
  }
  std::cout<<"MSearchRespone : "<<MSearchRespone<<std::endl;
  char* localtionBeginPos=strcasestr(MSearchRespone,"Location");
  char* urlBeginPos=strcasestr(localtionBeginPos,"http");
  char* urlEndPos=strcasestr(urlBeginPos,"\r\n");
  char* ipStart=strcasestr(localtionBeginPos,"//")+2;
  char* ipEnd=strcasestr(ipStart,":");
  char* portStart=ipEnd+1;
  char* portEnd=strcasestr(portStart,"/");
  char* pathStart=portEnd;
  char* pathEnd=strcasestr(pathStart,"\r\n");
  info.ip.append(ipStart,ipEnd-ipStart);
  info.port.append(portStart,portEnd-portStart);


  std::string path;
  path.append(pathStart,pathEnd-pathStart);

  std::string xmlContent=httpGet(info.ip,info.port,path);
  int wanIPConnectionServiceStart=xmlContent.find("urn:schemas-upnp-org:service:WANIPConnection:");
  int wanIPConnectionServiceEnd=xmlContent.find("</service>",wanIPConnectionServiceStart);
  int controlUrlStart=xmlContent.find("<controlURL>",wanIPConnectionServiceStart)+sizeof("<controlURL>")-1;
  int controlUrlEnd=xmlContent.find("</controlURL>",controlUrlStart);
  info.ctlurl=xmlContent.substr(controlUrlStart,controlUrlEnd-controlUrlStart);
  //std::cout<<"Info.ip : "<<info.ip<<std::endl;
  //std::cout<<"Info.ctlUrl : "<<info.ctlurl<<std::endl;
  return info;
}


int main(int argc,char** argv)
{
  RouterInfo i=GetRouterInfo(); 
  std::string externIPAddressStr=getExternalIPAddress(i.ip,i.port,i.ctlurl);
  std::cout<<"extern ip : "<<externIPAddressStr<<std::endl;
  /*
     PortMapInfo inputinfo;
     inputinfo.externPort="33446";
     inputinfo.localPort="44556";
     inputinfo.localIP="192.168.1.128";
     inputinfo.newEnable="1";
     inputinfo.protoType="UDP";
     inputinfo.description="From setupnp";
     addPortMapping(i.ip,i.port,i.ctlurl,inputinfo);
     */
  PortMapInfo info=getGenericPortMappingEntry(i.ip,i.port,i.ctlurl,0);
  std::cout<<"mapping : "<<info.externPort<<" -------> "<<info.localIP<<":"<<info.localPort<<" "<<info.description<<std::endl;
  return 0;
}

