// //
// // Created by Joe on 26-4-12.
// //
// #include <httplib.h>
// #include <iostream>
//
// int main() {
//     httplib::Client cli("localhost", 8080);
//
//     auto res = cli.Get("/hello");
//
//     if (res) {
//         std::cout << "Status: " << res->status << std::endl;
//         std::cout << "Body: " << res->body << std::endl;
//
//     } else {
//         std::cout << "Request failed!" << std::endl;
//     }
//     return 0;
// }