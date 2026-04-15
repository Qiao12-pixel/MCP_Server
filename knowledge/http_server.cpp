// //
// // Created by Joe on 26-4-12.
// //
// #include <httplib.h>
// #include <iostream>
//
// int main() {
//     httplib::Server server;
//
//     //处理GET请求
//     server.Get("/hello", [](const httplib::Request& req, httplib::Response& res) {
//        res.set_content("Hello, world", "text/plain");
//     });
//     server.Get("/greet", [](const httplib::Request& req, httplib::Response& res) {
//         std::string name = req.get_param_value("name");
//         if (name.empty()) {
//             name = "Guest";
//         }
//         res.set_content("Hello, " + name + "!", "text/plain");
//     });
//
//     std::cout << "Server running at http://localhost:8080" << std::endl;
//     server.listen("0.0.0.0", 8080);
// }