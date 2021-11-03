#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <string>
#include <functional>


struct MemoryStruct
{
    char *memory;
    size_t size;
};

using Callback = std::function<void(const MemoryStruct &m)>;

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize;
    struct MemoryStruct *mem;

	printf("%s nmemb:%x\n", __func__, nmemb);
    realsize = size * nmemb;
    mem = (struct MemoryStruct *)userp;

	printf("zz %s nmemb:%lx userp:%lx realsize:%lx \n",__func__, (unsigned long)nmemb, (unsigned long)userp, (unsigned long)realsize);
    mem->memory = (char*)realloc(mem->memory, mem->size + realsize);
    if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
	printf("%s nmemb:%x\n", __func__, nmemb);
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

class CurlDownload
{
public:
    CurlDownload()
	{
    	chunk.memory = NULL;
	}
    CURLcode Initialize()
    {
        CURLcode error;

        curl_global_init(CURL_GLOBAL_ALL);
        /* init the curl session */
        curl_handle = curl_easy_init();
        ///* specify URL to get */
        //curl_easy_setopt(curl_handle, CURLOPT_URL, const_cast<char*>(strUrl.c_str()));
        ////curl_easy_setopt(curl_handle, CURLOPT_URL, "http://10.66.91.15:7777/ld/smog/2612_src.jpg");

        /* send all data to this function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);


        /* some servers don't like requests that are made without a user-agent
        field, so we provide one */
        //curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // https, skip the verification of the server's certificate.
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

        /* 设置连接超时,单位:毫秒 */
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 1000L);

        // add by yexiaoyogn 10 second time out 
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 2000);

        //add yexiaoyong set time out
        curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 3);
    }

    void SetCallback(Callback cb)
    {
        callback = cb;
    }

    CURLcode Download(std::string strUrl)
    {
        CURLcode res;

        /* specify URL to get */
        curl_easy_setopt(curl_handle, CURLOPT_URL, const_cast<char*>(strUrl.c_str()));

		curl_easy_setopt (curl_handle, CURLOPT_RANGE, "0-300");
        //curl_easy_setopt(curl_handle, CURLOPT_URL, "http://10.66.91.15:7777/ld/smog/2612_src.jpg");

        chunk.size = 0;

        /* get it! */
        res = curl_easy_perform(curl_handle);
        /* check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            /*
            * Now, our chunk.memory points to a memory block that is chunk.size
            * bytes big and contains the remote file.
            *
            * Do something nice with it!
            */
            printf("%lu bytes retrieved\n", (long)chunk.size);
        }

        callback(chunk);
    }

    void Finish()
    {
        /* cleanup curl stuff */
        curl_easy_cleanup(curl_handle);
        free(chunk.memory);
        /* we're done with libcurl, so clean it up */
        curl_global_cleanup();
    }

private:
    CURL *curl_handle;
    MemoryStruct chunk;
    Callback callback;
};

void CB(const MemoryStruct &m)
{
    static int cnt = 0;
    printf("CB: cnt:%d, %lu bytes retrieved\n", cnt++, (long)m.size);
}

int main(int argc, char **argv[])
{
    CurlDownload cd;
    cd.Initialize();
    cd.SetCallback(CB);
    //while (true)
    //{
        //cd.Download("https://10.66.91.15/ld/smog/2612_src.jpg");
        cd.Download("https://img-blog.csdnimg.cn/20210509103429488.png");
        //cd.Download("https://img-blog.csdnimg.cn/20210509103429488.png");
        //cd.Download("http://10.66.91.15:7777/ld/smog/2612_src.jpg");
    //}
    
    cd.Finish();
	return 0;
}

