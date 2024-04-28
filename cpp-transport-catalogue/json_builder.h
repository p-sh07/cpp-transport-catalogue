#include "json.h"

#include <stack>
#include <string>

namespace json {

class Builder {
private:
    class KeyReturnItem;
    class KeyValReturnItem;
    class ArrayReturnItem;
    class MapReturnItem;
    
public:
    ///Конструктор по умолчанию
    Builder();
    ///Передать ключ в текущий словарь
    KeyReturnItem Key(std::string key);
    ///Передать значение в текущий Node
    Builder& Value(Node node);
    ///Начать новый соварь
    MapReturnItem StartMap();
    ///Начать новый массив
    ArrayReturnItem StartArray();
    ///Завершить текущий словарь
    Builder& EndMap();
    ///Завершить текущий массив
    Builder& EndArray();
    ///Вернуть готовый Json
    Node Build();
    
private:
    bool has_nodes_ = false;
    Node root_;
    std::stack<Node*> tree_;
    
    ///Текущий Node - массив, и удалось в него положить новый Node
    bool PushIfArray(Node node);
    ///Ожидается значение либо словарь/массив (новый Node)
    bool ValueExpected();
    ///Ожидается ключ словаря
    bool KeyExpected();
    
//===================== ReturnItmems =====================//
    class RetItm {
    public:
        ///Конструктор по умолчанию
        RetItm(Builder& bd);
        //Protected для избежания вызова методов базового класса вне контекста
    protected:
        ///Передать ключ в текущий словарь
        void Key(std::string key);
        ///Передать значение в текущий Node
        void Value(Node node);
        ///Начать новый соварь
        void StartMap();
        ///Начать новый массив
        void StartArray();
        ///Завершить текущий словарь
        void EndMap();
        ///Завершить текущий массив
        void EndArray();
        //Builder& GetBuilder();
        Builder& bldr_;
    };

    //Возврат после добавления ключа
    class KeyReturnItem : public RetItm {
    public:
        using RetItm::RetItm;
        
        KeyValReturnItem Value(Node node);
        MapReturnItem StartMap();
        ArrayReturnItem StartArray();
    };
    //После последовательности Key()->Value()->
    class KeyValReturnItem : public RetItm {
    public:
        using RetItm::RetItm;
        
        KeyReturnItem Key(std::string key);
        Builder& EndMap();
    };
    //Возврат после создания словаря
    class MapReturnItem : public RetItm {
    public:
        using RetItm::RetItm;
        
        KeyReturnItem Key(std::string key);
        Builder& EndMap();
    };
    //Возврат после создания массива
    class ArrayReturnItem : public RetItm {
    public:
        using RetItm::RetItm;
        
        ArrayReturnItem Value(Node node);
        MapReturnItem StartMap();
        ArrayReturnItem StartArray();
        Builder& EndArray();
    };
};

} //namespace json
