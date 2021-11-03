#define CURL_STATICLIB                //如果是静态库方式，需要包含这句

#include "curl\curl.h"
#include <iostream>
#include <list>
#include <string>

#ifdef _DEBUG
#pragma comment(lib,"libcurld.lib")
#else
#pragma comment(lib,"libcurl.lib")
#endif

#pragma comment ( lib, "ws2_32.lib" )
#pragma comment ( lib, "winmm.lib" )
#pragma comment ( lib, "wldap32.lib" )
#pragma comment(lib, "Advapi32.lib")


std::wstring AsciiToUnicode(const std::string& str)
{
    // 预算-缓冲区中宽字节的长度
    int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    // 给指向缓冲区的指针变量分配内存
    wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
    // 开始向缓冲区转换字节
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
    std::wstring ret_str = pUnicode;
    free(pUnicode);
    return ret_str;
}

std::string UnicodeToUtf8(const std::wstring& wstr)
{
    // 预算-缓冲区中多字节的长度
    int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    // 给指向缓冲区的指针变量分配内存
    char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
    // 开始向缓冲区转换字节
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
    std::string ret_str = pAssii;
    free(pAssii);
    return ret_str;
}

//ANSI转UTF8
std::string AsciiToUtf8(const std::string& str)
{
    return UnicodeToUtf8(AsciiToUnicode(str));
}

//UTF8转ANSI
std::string Utf8toAscii(const std::string strUTF8)
{
    std::string  strAnsi = "";
    //获取转换为多字节后需要的缓冲区大小，创建多字节缓冲区
    UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
    WCHAR *wszBuffer = new WCHAR[nLen + 1];
    nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
    wszBuffer[nLen] = 0;
    nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
    CHAR *szBuffer = new CHAR[nLen + 1];
    nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
    szBuffer[nLen] = 0;
    strAnsi = szBuffer;
    //清理内存
    delete[]szBuffer;
    delete[]wszBuffer;
    return strAnsi;
}

// reply of the requery
size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if (stream == NULL || ptr == NULL || size == 0)
        return 0;

    size_t realsize = size * nmemb;
    std::string *buffer = (std::string*)stream;
    if (buffer != NULL)
    {
        buffer->append((const char *)ptr, realsize);
    }
    return realsize;
    /*
    std::string *str = (std::string*)stream;
    (*str).append((char*)ptr, size*nmemb);
    return size * nmemb;
    */
}

/*
功能：get http数据
参数：url：请求字符串。如果请求带参数数据，直接拼凑到url后面；比如：http://127.0.0.1:8080/api/Accounts/Login?uername=admin&password=123
listRequestHeader：请求头数据列表。
bResponseIsWithHeaderData：bool类型，表示响应体中是否包含应答头数据。true，包含，false，不包含。如果包含的话，应答数据中包含Content-Type，Server等信息。
nConnectTimeout：连接超时时间，单位为秒；
nTimeout：读写数据超时时间，单位为秒
返回值：CURLcode
*/
CURLcode curl_get_req(const std::string &url, std::string &response, std::list<std::string> listRequestHeader, bool bResponseIsWithHeaderData = false, int nConnectTimeout = 10, int nTimeout = 10)
{
    // init curl
    CURL *curl = curl_easy_init();
    // res code
    CURLcode res;
    if (curl)
    {
        // set params
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
        //curl_easy_setopt(m_curl, CURLOPT_PORT, 8089);    //port
        curl_easy_setopt(curl, CURLOPT_POST, 0); // get reqest
        //构建HTTP报文头
        struct curl_slist* headers = NULL;
        if (listRequestHeader.size() > 0)
        {
            std::list<std::string>::iterator iter, iterEnd;
            iter = listRequestHeader.begin();
            iterEnd = listRequestHeader.end();
            for (iter; iter != iterEnd; iter++)
            {
                headers = curl_slist_append(headers, iter->c_str());
            }
            //headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
            //headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        if (bResponseIsWithHeaderData)
        {
            curl_easy_setopt(curl, CURLOPT_HEADER, 1);//响应体中是否包含了头信息，比如Content-Type:application/json;charset=UTF-8
        }
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout); // set transport and time out time
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
        // start request
        res = curl_easy_perform(curl);
        if (headers != NULL)
        {
            curl_slist_free_all(headers); //free the list again
        }
    }
    // release curl
    curl_easy_cleanup(curl);
    return res;
}

CURLcode curl_get_req_ex(CURL *curl, const std::string &url, std::string &response, std::list<std::string> listRequestHeader, bool bResponseIsWithHeaderData = false, int nConnectTimeout = 10, int nTimeout = 10)
{
    // res code
    CURLcode res;
    if (curl)
    {
        // set params
                curl_easy_reset(curl);
        /* enable TCP keep-alive for this transfer */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        /* keep-alive idle time to 120 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        /* interval time between keep-alive probes: 30 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
        //curl_easy_setopt(m_curl, CURLOPT_PORT, 8089);    //port
        curl_easy_setopt(curl, CURLOPT_POST, 0); // get reqest
        //构建HTTP报文头
        struct curl_slist* headers = NULL;
        if (listRequestHeader.size() > 0)
        {
            std::list<std::string>::iterator iter, iterEnd;
            iter = listRequestHeader.begin();
            iterEnd = listRequestHeader.end();
            for (iter; iter != iterEnd; iter++)
            {
                headers = curl_slist_append(headers, iter->c_str());
            }
            //headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
            //headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        if (bResponseIsWithHeaderData)
        {
            curl_easy_setopt(curl, CURLOPT_HEADER, 1);//响应体中是否包含了头信息，比如Content-Type:application/json;charset=UTF-8
        }
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout); // set transport and time out time
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
        // start request
        res = curl_easy_perform(curl);
        if (headers != NULL)
        {
            curl_slist_free_all(headers); //free the list again
        }
    }
    return res;
}

/*
功能：post http数据
参数：url：请求字符串，比如：http://127.0.0.1:8080/api/Accounts/Login
postParams：请求附带的参数，比如uername=admin&password=123
listRequestHeader：请求头数据列表。
bResponseIsWithHeaderData：bool类型，表示响应体中是否包含应答头数据。true，包含，false，不包含。如果包含的话，应答数据中包含Content-Type，Server等信息。
nConnectTimeout：连接超时时间，单位为秒；
nTimeout：读写数据超时时间，单位为秒
返回值：CURLcode
*/
CURLcode curl_post_req(const std::string &url, const std::string &postParams, std::string &response, std::list<std::string> listRequestHeader, bool bResponseIsWithHeaderData = false, int nConnectTimeout = 10, int nTimeout = 10)
{
    // init curl
    CURL *curl = curl_easy_init();
    // res code
    CURLcode res;
    if (curl)
    {
        // set params
        curl_easy_setopt(curl, CURLOPT_POST, 1); // post req
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
        //curl_easy_setopt(m_curl, CURLOPT_PORT, 8089);    //port
        curl_easy_setopt(curl, CURLOPT_POST, 1); // post reqest
        //构建HTTP报文头
        struct curl_slist* headers = NULL;
        if (listRequestHeader.size() > 0)
        {
            std::list<std::string>::iterator iter, iterEnd;
            iter = listRequestHeader.begin();
            iterEnd = listRequestHeader.end();
            for (iter; iter != iterEnd; iter++)
            {
                headers = curl_slist_append(headers, iter->c_str());
            }
            //headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
            //headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        else
        {
            headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str()); // params
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //返回的头部中有Location(一般直接请求的url没找到)，则继续请求Location对应的数据
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        if (bResponseIsWithHeaderData)
        {
            curl_easy_setopt(curl, CURLOPT_HEADER, 1);//响应体中是否包含了头信息，比如Content-Type:application/json;charset=UTF-8
        }
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
        // start request
        res = curl_easy_perform(curl);
        if (headers != NULL)
        {
            curl_slist_free_all(headers); //free the list again
        }
    }
    // release curl
    curl_easy_cleanup(curl);
    return res;
}

CURLcode curl_post_req_ex(CURL *curl, const std::string &url, const std::string &postParams, std::string &response, std::list<std::string> listRequestHeader, bool bResponseIsWithHeaderData = false, int nConnectTimeout = 10, int nTimeout = 10)
{
    // res code
    CURLcode res;
    if (curl)
    {
        // set params
                curl_easy_reset(curl);
        /* enable TCP keep-alive for this transfer */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        /* keep-alive idle time to 120 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        /* interval time between keep-alive probes: 30 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
        //curl_easy_setopt(m_curl, CURLOPT_PORT, 8089);    //port
        curl_easy_setopt(curl, CURLOPT_POST, 1); // post reqest
        //构建HTTP报文头
        struct curl_slist* headers = NULL;
        if (listRequestHeader.size() > 0)
        {
            std::list<std::string>::iterator iter, iterEnd;
            iter = listRequestHeader.begin();
            iterEnd = listRequestHeader.end();
            for (iter; iter != iterEnd; iter++)
            {
                headers = curl_slist_append(headers, iter->c_str());
            }
            //headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
            //headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        else
        {
            headers = curl_slist_append(headers, "Content-Type:application/x-www-form-urlencoded");
            if (headers != NULL)
            {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//设置http请求头信息
            }
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str()); // params
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //返回的头部中有Location(一般直接请求的url没找到)，则继续请求Location对应的数据
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        if (bResponseIsWithHeaderData)
        {
            curl_easy_setopt(curl, CURLOPT_HEADER, 1);//响应体中是否包含了头信息，比如Content-Type:application/json;charset=UTF-8
        }
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
        // start request
        res = curl_easy_perform(curl);
        if (headers != NULL)
        {
            curl_slist_free_all(headers); //free the list again
        }
    }
    return res;
}

//实例1
    curl_global_init(CURL_GLOBAL_ALL);

    //post获取数据
    std::string strResponse = "",strResponseAnsi = "";
        strResponse.clear();
    CURLcode res = curl_post_req("http://127.0.0.1:8080/api/Accounts/Login", "username=admin&password=123", strResponse);
    if (res == CURLE_OK)
    {
        std::string strToken = "";
        strResponseAnsi = Utf8toAscii(strResponse);
    }

    //get获取数据
        strResponse.clear();
    res = curl_get_req("http://127.0.0.1:8080/api/Accounts/Login?username=admin&password=123", strResponse);
    if (res == CURLE_OK)
    {
        int jj = 0;
    }

    curl_global_cleanup();
//实例2
    //post json数据
    CURL * curl = curl_easy_init();
    std::string strResponse = "", strResponseAnsi = "";
    char szRequestUrl[256] = { 0 };
    CURLcode res = CURLE_OK;
    sprintf_s(szRequestUrl, "%s/api/GPS/AddOne", "http://127.0.0.1:8080");
    std::string strPostParams = "";
    try
    {
        boost::property_tree::ptree ptroot;
        ptroot.put("deviceid", "12345678");
        ptroot.put<unsigned int>("deviceStatus", 0);
        ptroot.put<unsigned int>("alarmFlag", 0);
        ptroot.put("lng", fLongitude);
        ptroot.put("lat", fLatitude);
        ptroot.put("speed", 0);
        ptroot.put("direction", 0);
        ptroot.put<int>("altitude", 10);
        ptroot.put("gpsTime", "2018-10-10 12:00:01");
        std::stringstream sstream;
        boost::property_tree::write_json(sstream, ptroot);
        strPostParams = sstream.str();
        bSuccess = true;
    }
    catch (boost::property_tree::ptree_error pt)
    {
        pt.what();
    }
    if (bSuccess)
    {
      std::string strAuthorization = "admin---";
        std::string strRequestHeaders = strAuthorization;
        std::list<std::string> listRequestHeader;
        listRequestHeader.push_back(strRequestHeaders);
        listRequestHeader.push_back("Content-Type:application/json;charset=UTF-8");
        res = curl_post_req_ex(curl, szRequestUrl, strPostParams, strResponse, listRequestHeader);
        if (res == CURLE_OK)
        {
            bSuccess = true;
        }
    }

	curl_easy_cleanup(curl);
	return 0;
}

