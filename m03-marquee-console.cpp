// CSOPESY - S05 - GROUP 8
// M03 - MARQUEE CONSOLE

//          ***** REMOVE THIS PART AFTER *****
// ============================================================
//                             FLOW
// ============================================================
// ----- First -----
// User enters a command
// Is the command Valid?
// [YES] Process the Command
// [NO] Show "Command not recognized. Type 'help' to view the command list.", then ask for another command.

// ----- Second -----
// If user enters "help"
// Valid -> Show available commands

// ----- Third -----
// If user enters "start_marquee"
// IS TEXT ALREADY SET? (set_text <text>)
// [YES] Start Maquee Animation
// [NO] Show "The text field is empty. Please use 'set_text' first."

// ----- Fourth -----
// If user enters "stop_marquee"
// IS MARQUEE RUNNING?
// [YES] Stop Marquee Animation, Display "Marquee has stopped."
// [NO] Show "Marquee isn't running right now."

// ----- Fifth -----
// If user enters "set_text <text>"
// Set/Update the marquee text
// Display "Text has been updated."

// ----- Sixth -----
// If user enters "set_speed"
// Set/update the marquee speed
// VALID?
// [YES] Display "Speed set to # ms."
// [NO] If user enters 'set_speed abc', display "Invalid Speed. Try Again."
// [NO] If user 'set_speed -10', display "Invalid Speed. Speed must be grater than 0."

// -----Seventh -----
// If user enters "exit"
// Stop/exit the Program
// Display "Session Ending..."

#include <iostream> // Input/Output Stream, For std::cout, std::cin, std::endl
#include <string> // For std::string
#include <queue> // FIFO, For std::queue
#include <mutex> // Mutual Exclusion for multiple threads, For std::mutex, std::lock_guard
#include <atomic> // For atomic operations, For std::atomic
#include <thread> // Manage threads, For std::thread
#include <chrono> // C++'s time library, For std::chrono::milliseconds, std::this_thread::sleep_for
#include <cctype> // Character type functions, For std::isprint

// ============================================================
//                         SHARED STATE
// ============================================================

// boolean variable to control the running state of the program
std::atomic<bool> is_running{true};
std::atomic<bool> marquee_running{false};

// Shared state variables for marquee text and speed, protected by mutexes
std::string marquee_text;
std::mutex marquee_text_mutex;
int marquee_speed = 100;
std::mutex marquee_speed_mutex;

// Command queue and mutex for thread-safe access
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;

// User input and console prompt buffers, protected by mutexes
std::string current_input_buffer;
std::mutex input_buffer_mutex;
std::string prompt_display_buffer;
std::mutex prompt_mutex;

// ============================================================
//                  KEYBOARD / INPUT SYSTEM
// ============================================================

// Function to handle keyboard input in a separate thread
void keyboard_handler_thread_func() {
    while (is_running) {
        char ch = std::cin.get(); // Get a single character from standard input

        // Handle Enter key (newline)
        if (ch == '\n') {
            std::string command;
            // Lock the input buffer mutex to safely access the current input buffer
            {
                std::lock_guard<std::mutex> input_lock(input_buffer_mutex);
                // Move the current input buffer to the command variable and clear it for new input
                command = current_input_buffer;
                current_input_buffer.clear();
            }
            // If the command is not empty, push it to the command queue
            if (!command.empty()) {
                std::lock_guard<std::mutex> queue_lock(command_queue_mutex);
                command_queue.push(command);
            }
        }
        // Handle Backspace key (ASCII 8 or 127)
        else if (ch == '\b' || ch == 127) {
            // Lock the input buffer mutex to safely modify the current input buffer
            std::lock_guard<std::mutex> input_lock(input_buffer_mutex);
            // Remove the last character from the current input buffer if it's not empty
            if (!current_input_buffer.empty()) {
                current_input_buffer.pop_back();
            }
        }
        // Handle printable characters
        else if (std::isprint(static_cast<unsigned char>(ch))) {
            std::lock_guard<std::mutex> input_lock(input_buffer_mutex);
            current_input_buffer += ch; // Append the character to the current input buffer
        }
    }
}

// ============================================================
//                     COMMAND HANDLER
// ============================================================

// Function to handle commands from the command queue
void command_handler(const std::string &command_line) { // parameter: pointer to the command line string
    std::string output;

    // ========================================================
    // HELP
    // ========================================================

    if (command_line == "help") {
        output =
            "\nAvailable commands:\n"
            " help           - Show commands\n"
            " start_marquee  - Start marquee animation\n"
            " stop_marquee   - Stop marquee animation\n"
            " set_text       - Set new text\n"
            " set_speed      - Set new speed in milliseconds\n"
            " exit           - Terminate the console\n\n";
    }

    // ========================================================
    // START MARQUEE - UPDATE THIS
    // ========================================================

    else if (command_line == "start_marquee") {
        bool has_text;

        {
            std::lock_guard<std::mutex> lock(
                marquee_text_mutex);

            has_text = !marquee_text.empty();
        }

        if (!has_text) {
            output =
                "The text field is empty. Please use 'set_text' first.\n";
        } else {
            output =
                "Marquee is ready.\n"
                "Stored text: [" + marquee_text + "]\n";
        }
    }

    // ========================================================
    // STOP MARQUEE - UPDATE THIS
    // ========================================================

    else if (command_line == "stop_marquee") {
        if (marquee_running) {
            marquee_running = false;

            output =
                "Marquee has stopped.\n";
        } else {
            output =
                "Marquee isn't running right now.\n";
        }
    }

    // ========================================================
    // SET TEXT - UPDATE THIS
    // ========================================================

    else if (command_line == "set_text") {
        output =
            "Please provide text.\n"
            "Format: set_text <your text>\n";
    }

    else if (
        command_line.rfind("set_text ", 0) == 0)
    {
        // Get text after "set_text "
        std::string new_text = command_line.substr(9);

        if (new_text.empty()) {
            output =
                "Please provide text.\n"
                "Format: set_text <your text>\n";
        } else {
            // Store text
            {
                std::lock_guard<std::mutex> lock(
                    marquee_text_mutex);

                marquee_text = new_text;
            }

            // ------------------------------------------------
            // REMOVE THIS PART AFTER
            // Temporary display to verify that the input was successfully stored.
            // ------------------------------------------------

            output =
                "Text has been updated.\n"
                "Current text: [" + new_text + "]\n";
        }
    }

    // ========================================================
    // SET SPEED - UPDATE THIS
    // ========================================================

    else if (command_line == "set_speed") {
        output =
            "Please provide speed.\n"
            "Format: set_speed <milliseconds>\n";
    }

    // --------------------------------------------------------
    // SET SPEED WITH VALUE
    // --------------------------------------------------------

    else if (
        command_line.rfind("set_speed ", 0) == 0)
    {
        std::string speed_text = command_line.substr(10);

        // Check if speed is empty
        if (speed_text.empty()) {
            output =
                "Please provide speed.\n"
                "Format: set_speed <milliseconds>\n";
        } else {
            // try block to catch any exceptions from std::stoi meaning the input is not a valid integer
            try {
                // variable to track the position of the first invalid character in the string
                size_t position = 0; 
                // convert the speed_text string to an integer,
                // and store the position of the first invalid character in the position variable
                int speed = std::stoi(speed_text, &position); 

                // Check for extra characters
                if (position != speed_text.length()) {
                    output =
                        "Unsupported Speed Value.\n"
                        "Format: set_speed <milliseconds>\n";
                }
                // Check for negative/zero value
                else if (speed <= 0) {
                    output = "Unsupported Speed Value. Speed must be greater than 0.\n";
                }
                // Valid speed
                else {
                    // code block to limit the scope of the lock_guard,
                    // ensuring that the mutex is unlocked after updating the marquee_speed
                    { 
                        std::lock_guard<std::mutex> lock(marquee_speed_mutex);
                        marquee_speed = speed;
                    }
                    output = "Speed set to " + std::to_string(speed) + " ms.\n";
                }
            } 
            // catch block to handle any exceptions thrown by std::stoi,
            // indicating that the input is not a valid integer
            catch (...) { 
                output =
                    "Unsupported Speed Value.\n"
                    "Format: set_speed <milliseconds>\n";
            }
        }
    }

    // ========================================================
    // EXIT
    // ========================================================

    else if (command_line == "exit") {
        marquee_running = false;
        is_running = false;
        output = "Session Ending...\n";
    }

    // ========================================================
    // UNKNOWN COMMAND
    // ========================================================

    else { output = "Unknown command. Type 'help' for available commands.\n"; }

    // ========================================================
    // STORE OUTPUT
    // ========================================================

    // code block to limit the scope of the lock_guard,
    // ensuring that the mutex is unlocked after updating the prompt_display_buffer
    {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        prompt_display_buffer = output;
    }

    // ========================================================
    // DISPLAY OUTPUT
    // ========================================================

    // display the output to the console
    std::cout << output;
    // check if the program is still running before displaying the command prompt
    // and flush the output buffer to ensure that the prompt is displayed immediately
    if (is_running) {
        std::cout << "Command> ";
        std::cout.flush();
    }
}

// ============================================================
//               COMMAND INTERPRETER THREAD
// ============================================================

// Function to handle commands from the command queue in a separate thread
void command_interpreter_thread_func() {
    while (is_running) {
        std::string command; 
        // code block to limit the scope of the lock_guard,
        // ensuring that the mutex is unlocked after accessing the command_queue
        {
            std::lock_guard<std::mutex> lock(command_queue_mutex);

            if (!command_queue.empty()) {
                command = command_queue.front();
                command_queue.pop();
            }
        }

        if (!command.empty()) {
            command_handler(command);
        }
        // Sleep for a short duration to prevent busy waiting and reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ============================================================
//                            MAIN
// ============================================================

int main()
{
    std::cout << "=====================================\n";
    std::cout << "      CSOPESY - M03 MARQUEE\n";
    std::cout << "      S05 - GROUP 8\n";
    std::cout << "=====================================\n\n";
    std::cout << "Group Developer:\n\n";
    std::cout << "Abenojar Fredrikzen\n";
    std::cout << "Caya, Mary Faye\n";
    std::cout << "Diamante, Deo Zamir\n";
    std::cout << "Guiller, Gerylyn\n\n\n";
    std::cout << "Version Date: September 26, 2026\n\n";
    std::cout << "Type 'help' to see available commands.";
    std::cout << "\n\n Command> ";
    std::cout.flush();

    // Start the keyboard handler and command interpreter threads
    std::thread keyboard_thread(keyboard_handler_thread_func);
    std::thread command_thread(command_interpreter_thread_func);

    // Wait for the command interpreter thread to finish before exiting the program
    command_thread.join();
    marquee_running = false;
    keyboard_thread.detach();

    std::cout << "\nProgram terminated.\n";

    return 0;
}