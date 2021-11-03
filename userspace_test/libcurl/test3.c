#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>


struct MemoryStruct
{
    char *memory;
    size_t size;
};

struct MemoryStruct chunk;
/******************************************************************************
* Function:         static size_t WriteMemoryCallback
* Description:      
* Where:
* Return:           
* Error:
*****************************************************************************/
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize;
    realsize = size * nmemb;

	printf("zz %s realsize:%lx \n",__func__, (unsigned long)realsize);
    return realsize;
}

/******************************************************************************
* Function:         static int crul_link_init
* Description:      
* Where:
* Return:           
* Error:            
*****************************************************************************/
static CURL *crul_link_init(void)
{
	//CURLcode error;
    CURL *curl_handle;

	curl_global_init(CURL_GLOBAL_ALL);
	/* init the curl session */
	curl_handle = curl_easy_init();
	///* specify URL to get */

	/* send all data to this function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	//curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	//https, skip the verification of the server's certificate.
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

	/* 设置连接超时,单位:毫秒 */
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 1000L);

	// add by yexiaoyogn 10 second time out 
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 2000);

	//add yexiaoyong set time out
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 3);
	return curl_handle;
}

/******************************************************************************
* Function:    static void curl_download_data
* Description:
* Where:
* Return:
* Error:            
*****************************************************************************/
static void curl_download_data(CURL *curl_handle, char * url_link, size_t offset, size_t len)
{
	CURLcode res;

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, (const char*) url_link);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	if (len != 0)
		curl_easy_setopt(curl_handle, CURLOPT_RANGE, "0-300");

	/* get it! */
	res = curl_easy_perform(curl_handle);
	/* check for errors */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	} else {
		printf("bytes retrieved\n");
	}
    //callback(chunk);
}

int main(int argc, char *argv[])
{
	CURL *curl_handle;
	curl_handle = crul_link_init();
	curl_download_data(curl_handle, "https://img-blog.csdnimg.cn/20210509103429488.png", 0, 0);

	return 0;
}

