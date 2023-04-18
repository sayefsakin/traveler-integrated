/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
/* <DESC>
 * Shows how the write callback function can be used to download data into a
 * chunk of memory instead of storing it in a file.
 * </DESC>
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#define DEBUG 0

using namespace std;
using namespace rapidjson;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class UrlParser {
public:
    string baseUrl;
    string datasetId;
    string travelerApi;
    Document urlParameters;
    UrlParser(string url, Document params) {
        baseUrl = url;
        urlParameters.CopyFrom(params, urlParameters.GetAllocator());
        if(DEBUG) cout << "url parser initiated with values" << endl;
    }
    UrlParser() {
        baseUrl = "http://localhost:8000";
        urlParameters.SetNull();
        if(DEBUG) cout << "url parser initiated" << endl;
    }

    Document fetchContentFromURL() {
        Document fetchedData;
        fetchedData.SetNull();

        CURL *curl_handle;
        CURLcode res;
        curl_handle = curl_easy_init();

        string readBuffer;

        string urlWithParams = makeUrlString(curl_handle);
        if(urlWithParams.empty()) return fetchedData;

        curl_easy_setopt(curl_handle, CURLOPT_URL, urlWithParams.data());
        curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curl_handle);

        /* check for errors */
        if(res != CURLE_OK) { fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            if(DEBUG) cout << " trying to parse data " << endl;
            //if(DEBUG) cout << readBuffer << endl;
            fetchedData.Parse(readBuffer.data());
            if(DEBUG) cout << "parsing done" << endl;
        }
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return fetchedData;
    }
private:
    string makeUrlString(CURL *curl) {
        string urlString;
        if(baseUrl.empty()) { if(DEBUG) cout << "please set URL first" << endl; return urlString;}
        else urlString = baseUrl;

        if(!datasetId.empty()) urlString += "/datasets/" + datasetId;
        if(!travelerApi.empty()) urlString += "/" + travelerApi;
        if(DEBUG) cout << "I am here too " << endl;
        string urlWithParams = baseUrl;
        if(!urlParameters.IsNull()) {
            if(DEBUG) cout << "I am here " << endl;
            const Value& V = urlParameters;
            bool isFirst = true;
            for (Value::ConstMemberIterator iter = V.MemberBegin(); iter != V.MemberEnd(); ++iter){
                if(isFirst) {urlString += "?"; isFirst = false;}
                else urlString += "&";
                string paramKey = iter->name.GetString();
                string paramValue = curl_easy_escape(curl, iter->value.GetString(), strlen(iter->value.GetString()));
                urlString += paramKey + "=" + paramValue;
            }
        }
        cout << "Generated Url String: " << urlString << endl;
        return urlString;
    }
};

/*
int main(void)
{
  UrlParser urlparser;
  //urlparser.urlString = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/intervalHistograms?bins=100";
  urlparser.urlString = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/primitives";
  Document d = urlparser.fetchContentFromURL();
  if(DEBUG) cout << d["load_components_action"]["name"].GetString() << endl;
  return 0;
}*/