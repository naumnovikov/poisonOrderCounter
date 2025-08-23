#include <iostream>
#include <curl/curl.h>
#include <string>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>

class HTMLpageWithExchangeRate{
private:
    std::string HTMLcontent{};
    const std::string URL{"https://www.cbr.ru/currency_base/daily/"};
public:
    CURL* initializeHandle(){
        CURL* handle{curl_easy_init()};
        if (!handle){
            std::cerr << "Can't initialize a handle!\n";
            exit(1);
        }
        return handle;
    }
    
    void setHandleOptions(CURL* handle){
        curl_easy_setopt(handle, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &HTMLcontent);
    }
    
    void performHandle(CURL* handle){
        CURLcode result{curl_easy_perform(handle)};
        if (result != CURLE_OK){
            std::cerr << "Can't perform a handle!\n";
        }
    }
    
    void getPage(){
        CURL* handle{initializeHandle()};
        setHandleOptions(handle);
        performHandle(handle);
        curl_easy_cleanup(handle);
    }
    
    static size_t countDataSize(size_t sizeOfDataPart, size_t quantityOfParts){
        return sizeOfDataPart*quantityOfParts;
    }
    
    static void writeContentToBuffer(void* content, std::string* BufferForProcessedData, size_t sizeOfAllDataParts){
        BufferForProcessedData->append(reinterpret_cast<char*>(content), sizeOfAllDataParts);
    }
    
    static size_t writeCallback(void* content, size_t sizeOfDataPart, size_t quantityOfParts, std::string* BufferForProcessedData){
        size_t sizeOfAllDataParts{countDataSize(sizeOfDataPart, quantityOfParts)};
        writeContentToBuffer(content, BufferForProcessedData, sizeOfAllDataParts);
        return sizeOfAllDataParts;
    }
    
    bool canParse(){
        if (HTMLcontent.size() > INT_MAX){
            std::cerr << "Can't parse an HTML! HTML page is bigger than INT_MAX.\n";
            return false;
        }
        return true;
    }
    
    htmlDocPtr createHTMLDocPtr(){
        htmlDocPtr docPtr{htmlReadMemory(HTMLcontent.c_str(), static_cast<int>(HTMLcontent.size()), nullptr, nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING)};
        return docPtr;
    }
    
    xmlXPathObjectPtr createXPathObject(xmlXPathContextPtr XPathContextPtr){
        xmlXPathObjectPtr XPathObject = xmlXPathEvalExpression((xmlChar*)"//td", XPathContextPtr);
        return XPathObject;
    }
    
    void parseHTML(){
        if (!canParse()){
            return;
        }
        
        htmlDocPtr docPtr{createHTMLDocPtr()};
        if (!docPtr){
            std::cerr << "Can't create DocPtr!\n";
            return;
        }
        
        xmlXPathContextPtr XPathContextPtr{xmlXPathNewContext(docPtr)};
        if (!XPathContextPtr){
            xmlFreeDoc(docPtr);
            std::cerr << "Can't create XPathContextPtr!\n";
            return;
        }
        
        xmlXPathObjectPtr XPathObject{createXPathObject(XPathContextPtr)};
        if (!XPathObject) {
            std::cerr << "Can't create XPathObject!\n";
            xmlXPathFreeContext(XPathContextPtr);
            xmlFreeDoc(docPtr);
            return;
        }
        
        xmlNodeSetPtr nodes{XPathObject->nodesetval};
        int quantityOfNodes{xmlXPathNodeSetGetLength(nodes)};
        for (int i{0}; i < quantityOfNodes; ++i){
            xmlNodePtr node{xmlXPathNodeSetItem(nodes, i)};
            xmlChar* nodeContent{xmlNodeGetContent(node)};
            if (nodeContent && i==quantityOfNodes-1){
                std::cout << nodeContent << "\n";
            }
            xmlFree(nodeContent);
        }
        
        xmlXPathFreeObject(XPathObject);
        xmlXPathFreeContext(XPathContextPtr);
        xmlFreeDoc(docPtr);
    }
};


int main(){
    HTMLpageWithExchangeRate page;
    
    page.getPage();
    page.parseHTML();
}
