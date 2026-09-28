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
// [NO] Show "Command not recognized. Type 'help' to view the command list.", then ask for another
// command.

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

#include <algorithm>  // Provides functions like std::min()
#include <atomic>     // For atomic operations, For std::atomic
#include <cctype>     // Character type functions, For std::isprint
#include <chrono>     // C++'s time library, For std::chrono::milliseconds, std::this_thread::sleep_for
#include <iostream>   // Input/Output Stream, For std::cout, std::cin, std::endl
#include <mutex>      // Mutual Exclusion for multiple threads, For std::mutex, std::lock_guard
#include <queue>      // FIFO, For std::queue
#include <string>     // For std::string
#include <thread>     // Manage threads, For std::thread
#include <fstream>    // For file input/output, For std::ifstream, std::ofstream
#include <sstream>    // For string stream operations, For std::stringstream

#define NOMINMAX      // Stops windows.h from defining min/max macros (breaks std::min)
#include <windows.h>  // For enabling ANSI escape codes in the console
#include <conio.h>    // For _getch() to read keyboard input without waiting for Enter key

// global variables to control the polling intervals for keyboard and command interpreter threads
// polling means how often the thread checks for new input or commands.
// Lower values make the program more responsive but use more CPU.
int keyboard_polling_ms = 10;    // how fast to poll for keyboard input. 
                                 // (how fast keyboard_handler_thread_func checks for new key presses)
int interpreter_polling_ms = 10; // how fast to poll for command queue processing.
                                 // (how fast command_interpreter_thread_func checks for new commands in the queue)

// Shared program status
// atomic means that these variables can be safely read and written by multiple threads without causing data races.
std::atomic<bool> is_running{true};
std::atomic<bool> marquee_running{false};

// Marquee text
// mutex means that only one thread can access the marquee_text at a time, preventing data races.
std::string marquee_text;
std::mutex marquee_text_mutex;

// Marquee speed in milliseconds
// (how long to wait between each frame of the marquee animation)
// lower values make the marquee faster, higher values make it slower.
// 1000 ms = 1 second, 100 ms = 0.1 seconds, etc.
int marquee_speed = 100;
std::mutex marquee_speed_mutex;

// Marquee animation thread
std::thread marquee_thread;

// Commands waiting to be processed
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;

// Stores what the user is currently typing
std::string current_input_buffer;
std::mutex input_buffer_mutex;

// Prevents multiple threads from writing to the console at once.
// LOCK ORDER: always take console_mutex BEFORE input_buffer_mutex.
std::mutex console_mutex;

// True while the line directly above the prompt is reserved for the marquee.
// Only read/write this while holding console_mutex.
bool marquee_line_reserved = false;

// Lets the console understand ANSI escape codes (\033[...) on Windows
void enable_ansi_escape_codes() {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;

    if (!GetConsoleMode(handle, &mode)) {
        return;
    }

    SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

bool load_config(const std::string &filename = "config.txt") {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments starting with #
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key, value;

        // Split the line into key and value based on '='
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            // Trim whitespace (optional helper or basic trim)
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));

            // Convert only values that consist entirely of an integer.
            // if the value is not a valid integer, it will be ignored and defaults will be used.
            try {
                size_t parsed_chars = 0;
                int parsed_value = std::stoi(value, &parsed_chars);
                if (parsed_chars != value.size()) {
                    throw std::invalid_argument("Value is not an integer");
                }
                if (key == "default_marquee_speed_ms") {
                    std::lock_guard<std::mutex> lock(marquee_speed_mutex);
                    marquee_speed = parsed_value;
                } else if (key == "keyboard_polling_ms") {
                    keyboard_polling_ms = parsed_value;
                } else if (key == "interpreter_polling_ms") {
                    interpreter_polling_ms = parsed_value;
                }
            } catch (...) {
                // Fall back to defaults on parse errors
                // handled by ignoring the invalid value and using the default which is already set in the global variables.
            }
        }
    }
    return true;
}

// Returns the current input typed by the user
std::string get_current_input() {
    std::lock_guard<std::mutex> lock(input_buffer_mutex);

    return current_input_buffer;
}

// Redraws the Command> prompt with the current input.
// The cursor is always on the prompt line, so we only rewrite that line.
void draw_command_prompt() {
    std::lock_guard<std::mutex> console_lock(console_mutex);

    // The input is read INSIDE the console lock so two threads can never
    // draw an older copy of the input over a newer one.
    std::string input = get_current_input();

    // \r   = go to start of line
    // \033[K = clear from cursor to end of line (removes leftover characters
    //          without blanking the whole line first, which causes flicker)
    std::cout << "\r" << "Command> " << input << "\033[K";
    std::cout.flush();
}

// Sleep for the given time while still checking if
// the marquee has been stopped.
bool interruptible_sleep(int milliseconds) {
    int elapsed = 0;

    while (elapsed < milliseconds) {
        if (!is_running || !marquee_running) {
            return false;
        }

        int wait_time = std::min(10, milliseconds - elapsed);

        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

        elapsed += wait_time;
    }

    return true;
}

// Prints one frame of the marquee and then waits
// based on the current speed.
bool display_marquee_line(const std::string &text) {
    {
        std::lock_guard<std::mutex> console_lock(console_mutex);

        if (!is_running || !marquee_running || !marquee_line_reserved) {
            return false;
        }

        std::string input = get_current_input();

        // \033[1A = move cursor up 1 line (onto the marquee line)
        // \033[K  = clear from cursor to end of line
        // Then move down to the prompt line and redraw it with the input.
        std::cout << "\033[1A\r" << text << "\033[K\n"
                  << "\r" << "Command> " << input << "\033[K";
        std::cout.flush();
    }

    int speed;
    {
        std::lock_guard<std::mutex> lock(marquee_speed_mutex);
        speed = marquee_speed;
    }

    return interruptible_sleep(speed);
}

// Handles the marquee grow and delete animation
void marquee_animation() {
    while (is_running && marquee_running) {
        std::string text;

        {
            std::lock_guard<std::mutex> lock(marquee_text_mutex);

            text = marquee_text;
        }

        // Wait if there is no text to display
        if (text.empty()) {
            if (!interruptible_sleep(10)) {
                return;
            }

            continue;
        }

        // Show the text one character at a time
        for (size_t i = 1; i <= text.length(); i++) {
            if (!is_running || !marquee_running) {
                return;
            }

            std::string display = text.substr(0, i);

            if (!display_marquee_line(display)) {
                return;
            }
        }

        // Remove one character at a time
        for (int i = static_cast<int>(text.length()) - 1; i >= 1; i--) {
            if (!is_running || !marquee_running) {
                return;
            }

            std::string display = text.substr(0, i);

            if (!display_marquee_line(display)) {
                return;
            }
        }

        // Short pause before starting the animation again
        int speed;

        {
            std::lock_guard<std::mutex> lock(marquee_speed_mutex);

            speed = marquee_speed;
        }

        if (!interruptible_sleep(speed)) {
            return;
        }
    }
}

// Reads keyboard input and stores it in the input buffer
void keyboard_handler_thread_func() {
    while (is_running) {
        if (_kbhit()) {
            int ch = _getch();

            // Special keys (arrows, F-keys, etc.) send two codes.
            // Read and ignore the second one so it isn't typed as a letter.
            if (ch == 0 || ch == 0xE0) {
                _getch();
                continue;
            }

            // Submit the command when Enter is pressed
            if (ch == '\r' || ch == '\n') {
                std::string command;
                {
                    std::lock_guard<std::mutex> lock(input_buffer_mutex);
                    command = current_input_buffer;
                    current_input_buffer.clear();
                }

                if (!command.empty()) {
                    std::lock_guard<std::mutex> lock(command_queue_mutex);
                    command_queue.push(command);
                }
                // The command handler will redraw the prompt
            }

            // Remove the last character when Backspace is pressed
            else if (ch == '\b' || ch == 127) {
                {
                    std::lock_guard<std::mutex> lock(input_buffer_mutex);

                    if (!current_input_buffer.empty()) {
                        current_input_buffer.pop_back();
                    }
                }

                draw_command_prompt();
            }

            // Add printable characters to the input buffer
            else if (std::isprint(ch)) {
                {
                    std::lock_guard<std::mutex> lock(input_buffer_mutex);

                    current_input_buffer += static_cast<char>(ch);
                }

                draw_command_prompt();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(keyboard_polling_ms));
    }
}

// Prints a command's result and restores the prompt.
//
// Screen layout is always one of:
//   marquee NOT running:   [output...]
//                          Command> _
//
//   marquee running:       [marquee line]
//                          Command> _
//
// marquee_line_reserved tells us which layout is currently on screen.
void render_result(const std::string &output) {
    std::lock_guard<std::mutex> console_lock(console_mutex);

    // If a marquee line exists, start from it so it gets replaced too
    // \033[1A = move cursor up 1 line
    if (marquee_line_reserved) {
        std::cout << "\033[1A";
    }

    // \r     = start of line
    // \033[J = clear from cursor to end of screen. This wipes the marquee
    //          line AND the old prompt (which still shows the typed command).
    std::cout << "\r\033[J" << output;

    marquee_line_reserved = false;

    // If the marquee keeps running, reserve a fresh line above the prompt
    if (marquee_running && is_running) {
        std::cout << "\n";
        marquee_line_reserved = true;
    }

    if (is_running) {
        std::cout << "Command> " << get_current_input();
    }

    std::cout.flush();
}

// Processes commands entered by the user
void command_handler(const std::string &command_line) {
    std::string output;

    // Set when start_marquee succeeds. The thread is launched AFTER the
    // marquee line has been reserved on screen, so it can't draw too early.
    bool launch_marquee_thread = false;

    // Display available commands
    if (command_line == "help") {
        output = "\nAvailable commands:\n"
                 " help           - Show commands\n"
                 " start_marquee  - Start marquee animation\n"
                 " stop_marquee   - Stop marquee animation\n"
                 " set_text       - Set new text\n"
                 " set_speed      - Set marquee speed in milliseconds\n"
                 " exit           - Terminate the console\n\n";
    }

    // Start the marquee animation
    else if (command_line == "start_marquee") {
        bool has_text;

        {
            std::lock_guard<std::mutex> lock(marquee_text_mutex);

            has_text = !marquee_text.empty();
        }

        if (!has_text) {
            output = "The text field is empty. "
                     "Please use 'set_text' first.\n";
        } else if (marquee_running) {
            output = "Marquee is already running.\n";
        } else {
            marquee_running = true;
            launch_marquee_thread = true;
            output = "";
        }
    }

    // Stop the marquee animation
    else if (command_line == "stop_marquee") {
        if (marquee_running) {
            marquee_running = false;

            // Join BEFORE touching the console: the marquee thread needs
            // console_mutex to finish its last frame.
            if (marquee_thread.joinable()) {
                marquee_thread.join();
            }

            output = "Marquee has stopped.\n";
        } else {
            output = "Marquee isn't running right now.\n";
        }
    }

    // Handle set_text without a value
    else if (command_line == "set_text") {
        output = "Please provide text.\n"
                 "Format: set_text <your text>\n";
    }

    // Set the marquee text
    else if (command_line.rfind("set_text ", 0) == 0) {
        std::string new_text = command_line.substr(9);

        if (new_text.empty()) {
            output = "Please provide text.\n"
                     "Format: set_text <your text>\n";
        } else {
            {
                std::lock_guard<std::mutex> lock(marquee_text_mutex);

                marquee_text = new_text;
            }

            output = "Text has been updated.\n";
        }
    }

    // Handle set_speed without a value
    else if (command_line == "set_speed") {
        output = "Please provide speed.\n"
                 "Format: set_speed <milliseconds>\n";
    }

    // Change the marquee speed
    else if (command_line.rfind("set_speed ", 0) == 0) {
        std::string speed_text = command_line.substr(10);

        if (speed_text.empty()) {
            output = "Please provide speed.\n"
                     "Format: set_speed <milliseconds>\n";
        } else {
            try {
                size_t position = 0;

                int speed = std::stoi(speed_text, &position);

                // Check for extra characters in the input
                if (position != speed_text.length()) {
                    output = "Invalid Speed. "
                             "Try Again.\n";
                }

                // Speed must be greater than zero
                else if (speed <= 0) {
                    output = "Invalid Speed. "
                             "Speed must be greater than 0.\n";
                } else {
                    {
                        std::lock_guard<std::mutex> lock(marquee_speed_mutex);

                        marquee_speed = speed;
                    }

                    output = "Speed set to " + std::to_string(speed) + " ms.\n";
                }
            } catch (...) {
                output = "Invalid Speed. "
                         "Try Again.\n";
            }
        }
    }

    // Exit the program
    else if (command_line == "exit") {
        marquee_running = false;
        is_running = false;

        if (marquee_thread.joinable()) {
            marquee_thread.join();
        }

        output = "\nSession Ending...\n";
    }

    // Handle commands that are not recognized
    else {
        output = "Unknown command. "
                 "Type 'help' for available commands.\n";
    }

    // Display the result and restore the prompt
    render_result(output);

    // Now that the marquee line exists on screen, start the animation
    if (launch_marquee_thread) {
        marquee_thread = std::thread(marquee_animation);
    }
}

// Takes commands from the queue and processes them
void command_interpreter_thread_func() {
    while (is_running) {
        std::string command;

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

        // Prevent the thread from constantly checking the queue
        std::this_thread::sleep_for(std::chrono::milliseconds(interpreter_polling_ms));
    }
}

int main() {
    enable_ansi_escape_codes();

    // Load configurations from config.txt at startup
    const bool config_loaded = load_config("config.txt");
    if (config_loaded) {
        std::cout << "[CONFIG LOADED SUCCESS]\n";
    } else {
        std::cout << "[CONFIG LOAD FAILED] Using default settings.\n";
    }
    std::cout << "  - Initial Marquee Speed : " << marquee_speed << " ms\n"
              << "  - Keyboard Polling Rate : " << keyboard_polling_ms << " ms\n"
              << "  - Interpreter Polling   : " << interpreter_polling_ms << " ms\n"
              << "=====================================\n\n";

    // Initial console display
    std::cout << "=====================================\n"
              << "      CSOPESY - M03 MARQUEE\n"
              << "      S05 - GROUP 8\n"
              << "=====================================\n\n";

    std::cout << "Group Developer:\n\n"
              << "Abenojar, Fredrikzen\n"
              << "Caya, Mary Faye\n"
              << "Diamante, Deo Zamir\n"
              << "Guiller, Gerylyn\n\n\n";

    std::cout << "Version Date: September 28, 2026\n\n";

    std::cout << "Type 'help' to see available commands.\n\n";

    std::cout << "Command> ";

    std::cout.flush();

    // Start the input and command threads
    std::thread keyboard_thread(keyboard_handler_thread_func);

    std::thread command_thread(command_interpreter_thread_func);

    // Wait until the command thread finishes
    command_thread.join();

    // Stop the marquee before exiting
    marquee_running = false;

    if (marquee_thread.joinable()) {
        marquee_thread.join();
    }

    // The keyboard thread may be waiting for input,
    // so it is detached when the program ends.
    keyboard_thread.detach();

    std::cout << "\nProgram terminated.\n";

    return 0;
}
