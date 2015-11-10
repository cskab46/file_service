#ifndef SERVER_PROXY_H
#define SERVER_PROXY_H

//struct FileLock;

//!< Proxies operations requested by clients, if lead is true. Exits when quit is true.
void Proxy(const bool &lead, const bool &quit);

//!< Returns the current locks mapping of the proxy.
//const map<string, FileLock> &GetLocksMapping();

#endif // SERVER_PROXY_H
