// CSOPESY - S05
// Marquee Project - Command Line Interface Exercise

#include <iostream>
#include <string>

int main() {
    std::string command;
    std::string marqueeText;
    int marqueeSpeed = 100;

    // Welcome header
    std::cout << "Welcome to CSOPESY!\n\n";

    std::cout << "Group developer:\n";
    std::cout << "Caya, Mary Faye\n";
    std::cout << "Member 2\n";
    std::cout << "Member 3\n";
    std::cout << "Member 4\n\n";

    std::cout << "Version date: 2026-09-18\n\n";

    // Main command loop
    while (true) {
        std::cout << "Command> ";
        std::getline(std::cin, command);

        // HELP
        if (command == "help") {
            std::cout << "help - displays the commands and its description\n";
            std::cout << "start_marquee - starts the marquee \"animation\"\n";
            std::cout << "stop_marquee - stops the marquee \"animation\"\n";
            std::cout << "set_text - accepts a text input and displays it as a marquee\n";
            std::cout << "set_speed - sets the marquee animation refresh in milliseconds\n";
            std::cout << "exit - terminates the console\n";
        }

        // SET TEXT
        else if (command.rfind("set_text ", 0) == 0) {
            marqueeText = command.substr(9);

            std::cout << "Text saved for marquee: "
                      << marqueeText << "\n";
        }

        // START MARQUEE
        else if (command == "start_marquee") {
            std::cout << "Starting marquee...\n";
        }

        // STOP MARQUEE
        else if (command == "stop_marquee") {
            std::cout << "Stopping marquee...\n";
        }

        // SET SPEED
        else if (command.rfind("set_speed ", 0) == 0) {
            std::string speedText = command.substr(10);

            try {
                marqueeSpeed = std::stoi(speedText);
                std::cout << "Marquee speed set to "
                          << marqueeSpeed << " ms\n";
            }
            catch (...) {
                std::cout << "Invalid speed. Please enter a number.\n";
            }
        }

        // EXIT
        else if (command == "exit") {
            std::cout << "Terminating console...\n";
            break;
        }

        // UNKNOWN COMMAND
        else {
            std::cout << "Unknown command. Please try again.\n";
        }
    }

    return 0;
}