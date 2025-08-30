#include <iostream>
#include <curl/curl.h>
#include <string>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>

class HTMLpageWithExchangeRate{
private:
    std::string HTMLcontent{};
    xmlChar* yuanExchangeRate{};
    CURLcode handleCondition{};
    const std::string URL{"https://www.cbr.ru/currency_base/daily/"};
public:
    
    void getPage(){
        CURL* handle{initializeHandle()};
        setHandleOptions(handle);
        performHandle(handle);
        curl_easy_cleanup(handle);
    }
    
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
    
    static size_t writeCallback(void* content, size_t sizeOfDataPart, size_t quantityOfParts, std::string* BufferForProcessedData){
        size_t sizeOfAllDataParts{countDataSize(sizeOfDataPart, quantityOfParts)};
        writeContentToBuffer(content, BufferForProcessedData, sizeOfAllDataParts);
        return sizeOfAllDataParts;
    }
    
    static size_t countDataSize(size_t sizeOfDataPart, size_t quantityOfParts){
        return sizeOfDataPart*quantityOfParts;
    }
    
    static void writeContentToBuffer(void* content, std::string* BufferForProcessedData, size_t sizeOfAllDataParts){
        BufferForProcessedData->append(reinterpret_cast<char*>(content), sizeOfAllDataParts);
    }
    
    void performHandle(CURL* handle){
        CURLcode result{curl_easy_perform(handle)};
        handleCondition = result;
        if (result != CURLE_OK){
            std::cerr << "Can't perform a handle!\n";
            exit(1);
        }
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
            freeDocAndContext(XPathContextPtr, docPtr);
            return;
        }
        
        setYuanExchangeRate(XPathObject);
        
        finalCleaningObjCntxtDoc(XPathObject, XPathContextPtr, docPtr);
    }
    
    bool canParse(){
        if (HTMLcontent.size() > INT_MAX){
            std::cerr << "HTML-content is bigger than INT_MAX.";
            return false;
        } else if (handleCondition != CURLE_OK){
            std::cerr << "Handle condition is not CURLE_OK.";
            return false;
        }
        return true;
    }
    
    htmlDocPtr createHTMLDocPtr(){
        htmlDocPtr docPtr{htmlReadMemory(HTMLcontent.c_str(), static_cast<int>(HTMLcontent.size()), nullptr, nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING)};
        return docPtr;
    }
    
    xmlXPathObjectPtr createXPathObject(xmlXPathContextPtr XPathContextPtr){
        xmlXPathObjectPtr XPathObject{xmlXPathEvalExpression((xmlChar*)"//td", XPathContextPtr)};
        return XPathObject;
    }
    
    void freeDocAndContext(xmlXPathContextPtr& XPathContextPtr, htmlDocPtr& docPtr){
        xmlXPathFreeContext(XPathContextPtr);
        xmlFreeDoc(docPtr);
    }
    
    void setYuanExchangeRate(xmlXPathObjectPtr& XPathObject){
        xmlChar* yuanNodeContent{getYuanNodeContent(XPathObject)};
        if (yuanNodeContent != nullptr){
            yuanExchangeRate = xmlStrdup(yuanNodeContent);
        }
        xmlFree(yuanNodeContent);
    }
    
    xmlChar* getYuanNodeContent(xmlXPathObjectPtr& XPathObject){
        xmlNodeSetPtr foundNode{initializeNodes(XPathObject)};
        int quantityOfFoundNodes{countQuantityOfNodes(foundNode)};
        int yuanNodeIndex{quantityOfFoundNodes-1};
        xmlNodePtr yuanNode{xmlXPathNodeSetItem(foundNode, yuanNodeIndex)};
        xmlChar* yuanNodeContent{xmlNodeGetContent(yuanNode)};
        return yuanNodeContent;
    }
    
    xmlNodeSetPtr initializeNodes(xmlXPathObjectPtr& XPathObject){
        xmlNodeSetPtr nodes{XPathObject->nodesetval};
        return nodes;
    }
    
    int countQuantityOfNodes(xmlNodeSetPtr& nodes){
        return xmlXPathNodeSetGetLength(nodes);
    }
    
    
    
    
    void finalCleaningObjCntxtDoc(xmlXPathObjectPtr& XPathObject, xmlXPathContextPtr& XPathContextPtr, htmlDocPtr& docPtr){
        xmlXPathFreeObject(XPathObject);
        xmlXPathFreeContext(XPathContextPtr);
        xmlFreeDoc(docPtr);
    }
    
    void printYuanExchangeRate(){
        if (yuanExchangeRate == nullptr){
            std::cout << "Null exchange rate.\n";
            return;
        }
        std::cout << yuanExchangeRate << '\n';
    }
    
    xmlChar* returnYuanExchangeRate(){
        return yuanExchangeRate;
    }

};

void makeParse(HTMLpageWithExchangeRate& page);

double getYuanExchangeRateInDouble(HTMLpageWithExchangeRate& page);

int main(){
    HTMLpageWithExchangeRate page;
    
    makeParse(page);
    
    page.printYuanExchangeRate();
    
    double test{getYuanExchangeRateInDouble(page)};
    
    
}

void makeParse(HTMLpageWithExchangeRate& page){
    page.getPage();
    page.parseHTML();
}

double getYuanExchangeRateInDouble(HTMLpageWithExchangeRate& page){
    char* endPtr;
    const char* strValue = (const char*)page.returnYuanExchangeRate();
    double result = strtod(strValue, &endPtr);
    
    if (*endPtr == ',') {
        char* modified{strdup(strValue)};
        for (char* p{modified}; *p != '\0'; p++) {
            if (*p == ','){
                *p = '.';
            }
        }
            
        result = strtod(modified, &endPtr);
    }
    
    return result;
}

