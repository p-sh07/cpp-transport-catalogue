
#include "json_builder.h"

namespace json {
///Конструктор по умолчанию, добавляет пустой Node root в tree
Builder::Builder()
: root_()
, tree_({&root_}) {
}
Builder::KeyReturnItem Builder::Key(std::string key) {
    if(!KeyExpected()) {
        throw std::logic_error("A Map Key is not expected");
    }
    //add key & store ptr to value at tree top
    auto [it, _ ] = tree_.top()->GetMap().insert({std::move(key), Node()});
    tree_.push(&it->second);
    
    return KeyReturnItem(*this);
}

Builder& Builder::Value(Node node) {
    if(!ValueExpected()) {
        throw std::logic_error("A Value is not expected");
    }
    if(!PushIfArray(node)) {
        *tree_.top() = std::move(node);
        tree_.pop();
    }
    return *this;
}

Builder::MapReturnItem Builder::StartMap() {
    if(!ValueExpected()) {
        throw std::logic_error("Cannot make a Map");
    }
    if(!PushIfArray(Map{})) {
        tree_.top()->GetValue() = Map{};
    }
    return MapReturnItem(*this);
}

Builder::ArrayReturnItem Builder::StartArray() {
    if(!ValueExpected()) {
        throw std::logic_error("Cannot Start an Array");
    }
    if(!PushIfArray(Array{})) {
        tree_.top()->GetValue() = Array{};
    }
    
    return ArrayReturnItem(*this);
}

Builder& Builder::EndMap() {
    if(!KeyExpected() ) {
        throw std::logic_error("Cannot Finish a Map");
    }
    tree_.pop();
    return *this;
}

Builder& Builder::EndArray() {
    if(!tree_.empty() && !tree_.top()->IsArray()) {
        throw std::logic_error("Cannot Finish an Array");
    }
    tree_.pop();
    return *this;
}

Node Builder::Build() {
    if(!tree_.empty()) {
        throw std::logic_error("Unfinished Nodes left when calling Build");
    }
    return root_;
}

///Текущий Node - массив
bool Builder::PushIfArray(Node node) {
    if(tree_.top()->IsArray()) {
        bool push_to_stack = node.IsArray() || node.IsMap();
        tree_.top()->GetArray().push_back(std::move(node));
        
        if(push_to_stack) {
            tree_.push(&tree_.top()->GetArray().back());
        }
        return true;
    }
    return false;
}

///Ожидается значение либо словарь/массив (новый Node)
bool Builder::ValueExpected() {
    return !tree_.empty() && (tree_.top()->IsNull() || tree_.top()->IsArray());
}

///Ожидается ключ словаря
bool Builder::KeyExpected() {
    return !tree_.empty() && tree_.top()->IsMap();
}



//===================== ReturnItmems =====================//
Builder::RetItm::RetItm(Builder& bd)
: bldr_(bd) {}

///Передать ключ в текущий словарь
void Builder::RetItm::Key(std::string key) {
    bldr_.Key(key);
}

///Передать значение в текущий Node
void Builder::RetItm::Value(Node node) {
    bldr_.Value(node);
}
///Начать новый соварь
void Builder::RetItm::StartMap() {
    bldr_.StartMap();
}
///Начать новый массив
void Builder::RetItm::StartArray() {
    bldr_.StartArray();
}
///Завершить текущий словарь
void Builder::RetItm::EndMap() {
    bldr_.EndMap();
}
///Завершить текущий массив
void Builder::RetItm::EndArray() {
    bldr_.EndArray();
}

//Builder& Builder::RetItm::GetBuilder() {
//    return bldr_;
//}

//after Key is called:
Builder::KeyValReturnItem Builder::KeyReturnItem::Value(Node node) {
    RetItm::Value(std::move(node));
    return KeyValReturnItem(bldr_);
}
Builder::MapReturnItem Builder::KeyReturnItem::StartMap() {
    RetItm::StartMap();
    return MapReturnItem(bldr_);
}
Builder::ArrayReturnItem Builder::KeyReturnItem::StartArray() {
    RetItm::StartArray();
    return ArrayReturnItem(bldr_);
}

//after key->value->...
Builder::KeyReturnItem Builder::KeyValReturnItem::Key(std::string key) {
    RetItm::Key(std::move(key));
    return KeyReturnItem(bldr_);
}
Builder& Builder::KeyValReturnItem::EndMap() {
    RetItm::EndMap();
    return bldr_;
}

//after Map is started
Builder::KeyReturnItem Builder::MapReturnItem::Key(std::string key) {
    RetItm::Key(std::move(key));
    return KeyReturnItem(bldr_);
}
Builder& Builder::MapReturnItem::EndMap() {
    RetItm::EndMap();
    return bldr_;
}

//after array is started
Builder::ArrayReturnItem Builder::ArrayReturnItem::Value(Node node) {
    RetItm::Value(std::move(node));
    return ArrayReturnItem(bldr_);
}
Builder::MapReturnItem Builder::ArrayReturnItem::StartMap() {
    RetItm::StartMap();
    return MapReturnItem(bldr_);
}
Builder::ArrayReturnItem Builder::ArrayReturnItem::StartArray() {
    RetItm::StartArray();
    return ArrayReturnItem(bldr_);
}
Builder& Builder::ArrayReturnItem::EndArray() {
    RetItm::EndArray();
    return bldr_;
}

} //namespace json

