//
// Created by Joe on 26-4-12.
//
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

json obj = {
    {"name", "Alice"},
    {"age", 25},
    {"skills", {"C++", "Python"}},
    {"is_student", false}
};

// int main() {
//     //序列化
//     std::string str = obj.dump();//序列化
//     //反序列化
//     json parsed = json::parse(str);
//     std::string name = parsed["name"];
//     int age = parsed["age"];
//     std::cout << "name: " << name << " age: " << age << std::endl;
//
//     std::cout << "=============================" << std::endl;
//
//     std::string jsonString = R"({"city": "New York", "Population": 80000})";
//     json parsedJson = json::parse(jsonString);
//
//     std::cout << parsedJson.dump(2);
//     std::cout << parsedJson["city"];
//     return 0;
// }