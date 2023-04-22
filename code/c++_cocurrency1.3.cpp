#include <iostream>
#include <thread>
#include <string>
#include <vector>

// void edit_document(std::string const &filename) {
//     open_document_and_display(filename);
//     while(!done_editing()){
//         user_command cmd = get_user_input();
//         if(cmd == open_new_document){
//             std::string const new_file=get_filename();
//             std::thread t(edit_document,new_file);
//             t.detach();
//         }
//         else{
//             process_user_input(cmd);
//         }
//     }
// }

// void oops_again(widget_id w) {
//     widget_data data;
//     std::thread t(update_data_for_widget, w, data);
//     void display_status();
//     t.join();
//     process_widget_data(data);
// }