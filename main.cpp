#include <iostream>
#include <curl/curl.h>
#include <string>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <chrono>
#include <vector>

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


enum class typeOfClothing{
    shoes,
    clothes,
    accessory
};

struct positionStruct{
    double priceOfPositionInYuan{0};
    typeOfClothing positionTypeOfClothing{};
    decltype(std::chrono::system_clock::now()) timeWhenAddedToOrder{};
};

class order{
private:
    static std::unordered_map<unsigned int, positionStruct> allPositionsInOrder;
    static unsigned indexOfLastPositionAdded;
    
    
    decltype(std::chrono::system_clock::now()) timeOfAddingPosition{};
    double orderYuanExchangeRate{0};
    double orderPriceInYuan{0};
    double orderPriceInRub{0};
    double commissionPerOnePositionInOrder{1000};
public:
    void addPositionToOrder(){
        positionStruct position{};

        setPriceOfPositionInYuan(position);
        
        setPositionTypeOfClothing(position);
        
        setPositionTime(position);
        
        addPriceToOrderSum(position);
        
        putPositionToOrder(position);
    }
    
    void setPriceOfPositionInYuan(positionStruct& position){
        std::cout << "Type a price in yuan: ";
        double yuanPrice{0};
        enteringInputYuanPriceTillCorrect(yuanPrice);
        position.priceOfPositionInYuan = yuanPrice;
    }
    
    void enteringInputYuanPriceTillCorrect(double& yuanPrice){
        std::cin >> yuanPrice;
        while (std::cin.fail() || std::cin.peek() != '\n'){
            cleanCinConsoleAfterWrongInput();
            std::cerr << "Wrong input. Type the price in yuan: ";
            std::cin >> yuanPrice;
        }
    }
    
    void cleanCinConsoleAfterWrongInput(){
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    
    void setPositionTypeOfClothing(positionStruct& position){
        std::cout << "\nWhat a type of clothing?\n1)Shoes\n2)Clothes\n3)Accessory\n";
        int type{};
        std::cout << "Type: ";
        enteringInputTypeOfClothingTillCorrect(type);
        setPositionTypeOfClothing(type, position);
        std::cout << '\n';
    }
    
    void enteringInputTypeOfClothingTillCorrect(int& type){
        std::cin >> type;
        
        while (std::cin.fail() || std::cin.peek() != '\n' || type < 1 || type > 3){
            if (std::cin.fail() || std::cin.peek() != '\n'){
                std::cerr << "Wrong input. Choose a type of clothing: ";
            }
            if ((type < 1 || type > 3) && (!(std::cin.fail() || std::cin.peek() != '\n'))){
                std::cerr << "Type can't be less than 1 and more than 3. Choose a type of clothing: ";
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> type;
        }
    }
    
    void setPositionTypeOfClothing(int type, positionStruct& position){
        switch (type) {
            case 1:
                position.positionTypeOfClothing = typeOfClothing::shoes;
                break;
            case 2:
                position.positionTypeOfClothing = typeOfClothing::clothes;
                break;
            case 3:
                position.positionTypeOfClothing = typeOfClothing::accessory;
                break;
            default:
                break;
        }
    }
    
    void setPositionTime(positionStruct& position){
        position.timeWhenAddedToOrder = std::chrono::system_clock::now();
    }
    
    void addPriceToOrderSum(positionStruct& position){
        orderPriceInYuan+=position.priceOfPositionInYuan;
    }
    
    void putPositionToOrder(positionStruct& position){
        allPositionsInOrder[indexOfLastPositionAdded]=position;
    }
    
    enum class continueAdding{
        Yes,
        No
    };
    
    void makingOrder(){
        continueAdding ifContinueAdding{continueAdding::No};
        do {
            addPositionToOrder();
            ++indexOfLastPositionAdded;
            std::cout << "Want to add more in order?(Yes/No) ";
            std::string choiceOfConinuingAdding{};
            std::cin >> choiceOfConinuingAdding;
            while (choiceOfConinuingAdding != "Yes" && choiceOfConinuingAdding != "No"){
                std::cerr << "Wrong input. The answer should be Yes or No.\nWant to add more in order?(Yes/No) ";
                std::cin >> choiceOfConinuingAdding;
            }
            std::cout << '\n';
            if (choiceOfConinuingAdding == "Yes"){
                ifContinueAdding = continueAdding::Yes;
            }else if (choiceOfConinuingAdding == "No"){
                ifContinueAdding = continueAdding::No;
            }
        } while (ifContinueAdding == continueAdding::Yes);
        setPriceOfOrderInRub();
        
    }
    
    void printAllPositionsInOrder(){
        for (unsigned i{0}; i < indexOfLastPositionAdded; ++i){
            std::cout << static_cast<int>(allPositionsInOrder[i].positionTypeOfClothing)+1 << '\n';
            std::cout << allPositionsInOrder[i].priceOfPositionInYuan << '\n';
            std::cout << allPositionsInOrder[i].timeWhenAddedToOrder << '\n';
        }
    }
    
    double getOrderPriceInRub(){
        return orderPriceInRub;
    }
    
    void setYuanExchangeRate(double yuanExchangeRate){
        orderYuanExchangeRate = yuanExchangeRate;
    }
    
    void setPriceOfOrderInRub(){
        orderPriceInRub = orderYuanExchangeRate*orderPriceInYuan;
    }
};

std::unordered_map<unsigned int, positionStruct> order::allPositionsInOrder;
unsigned int order::indexOfLastPositionAdded = 0;


void makeParse(HTMLpageWithExchangeRate& page);

double getYuanExchangeRateInDouble(HTMLpageWithExchangeRate& page);

int main(){
    HTMLpageWithExchangeRate page;
    makeParse(page);
    double yuanExchangeRate{getYuanExchangeRateInDouble(page)};
    
    order orderObj;
    orderObj.setYuanExchangeRate(yuanExchangeRate);
    orderObj.makingOrder();
    double orderInRub{orderObj.getOrderPriceInRub()};
    std::cout << orderInRub << '\n';
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

