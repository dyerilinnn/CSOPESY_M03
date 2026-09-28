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
#include <algorithm> // Provides functions like std::min()

// Shared program status
std::atomic<bool> is_running{true};
std::atomic<bool> marquee_running{false};

// Marquee text
std::string marquee_text;
std::mutex marquee_text_mutex;

// Marquee speed in milliseconds
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

// Prevents multiple threads from writing to the console at once
std::mutex console_mutex;


// Returns the current input typed by the user
std::string get_current_input()
{
    std::lock_guard<std::mutex> lock(
        input_buffer_mutex);

    return current_input_buffer;
}


// Redraws the Command> prompt with the current input
void draw_command_prompt()
{
    std::string input;

    {
        std::lock_guard<std::mutex> lock(
            input_buffer_mutex);

        input = current_input_buffer;
    }

    std::lock_guard<std::mutex> lock(
        console_mutex);

    // Clear the current line and redraw the prompt
    std::cout
        << "\033[2K\r"
        << "Command> "
        << input;

    std::cout.flush();
}


// Sleep for the given time while still checking if
// the marquee has been stopped.
bool interruptible_sleep(int milliseconds)
{
    int elapsed = 0;

    while (elapsed < milliseconds)
    {
        if (!is_running || !marquee_running)
        {
            return false;
        }

        int wait_time =
            std::min(10, milliseconds - elapsed);

        std::this_thread::sleep_for(
            std::chrono::milliseconds(wait_time));

        elapsed += wait_time;
    }

    return true;
}


// Prints one frame of the marquee and then waits
// based on the current speed.
bool display_marquee_line(
    const std::string& text)
{
    if (!is_running || !marquee_running)
    {
        return false;
    }

    std::string input;

    {
        std::lock_guard<std::mutex> lock(
            input_buffer_mutex);

        input = current_input_buffer;
    }

    {
        std::lock_guard<std::mutex> lock(
            console_mutex);

        // Clear the current prompt before printing the frame
        std::cout
            << "\033[2K\r";

        // Print the current marquee text
        std::cout
            << text
            << '\n';

        // Keep the command prompt below the marquee
        std::cout
            << "Command> "
            << input;

        std::cout.flush();
    }

    int speed;

    {
        std::lock_guard<std::mutex> lock(
            marquee_speed_mutex);

        speed = marquee_speed;
    }

    return interruptible_sleep(speed);
}


// Handles the marquee grow and delete animation
void marquee_animation()
{
    while (is_running && marquee_running)
    {
        std::string text;

        {
            std::lock_guard<std::mutex> lock(
                marquee_text_mutex);

            text = marquee_text;
        }

        // Wait if there is no text to display
        if (text.empty())
        {
            if (!interruptible_sleep(10))
            {
                return;
            }

            continue;
        }

        // Show the text one character at a time
        for (size_t i = 1;
             i <= text.length();
             i++)
        {
            if (!is_running || !marquee_running)
            {
                return;
            }

            std::string display =
                text.substr(0, i);

            if (!display_marquee_line(display))
            {
                return;
            }
        }

        // Remove one character at a time
        for (int i =
                 static_cast<int>(
                     text.length()) - 1;
             i >= 1;
             i--)
        {
            if (!is_running || !marquee_running)
            {
                return;
            }

            std::string display =
                text.substr(0, i);

            if (!display_marquee_line(display))
            {
                return;
            }
        }

        // Short pause before starting the animation again
        int speed;

        {
            std::lock_guard<std::mutex> lock(
                marquee_speed_mutex);

            speed = marquee_speed;
        }

        if (!interruptible_sleep(speed))
        {
            return;
        }
    }
}


// Reads keyboard input and stores it in the input buffer
void keyboard_handler_thread_func()
{
    while (is_running)
    {
        char ch = std::cin.get();

        // Submit the command when Enter is pressed
        if (ch == '\n')
        {
            std::string command;

            {
                std::lock_guard<std::mutex> lock(
                    input_buffer_mutex);

                command =
                    current_input_buffer;

                current_input_buffer.clear();
            }

            if (!command.empty())
            {
                std::lock_guard<std::mutex> lock(
                    command_queue_mutex);

                command_queue.push(command);
            }

            // The command handler will redraw the prompt
            continue;
        }

        // Remove the last character when Backspace is pressed
        else if (ch == '\b' || ch == 127)
        {
            {
                std::lock_guard<std::mutex> lock(
                    input_buffer_mutex);

                if (!current_input_buffer.empty())
                {
                    current_input_buffer.pop_back();
                }
            }

            draw_command_prompt();
        }

        // Add printable characters to the input buffer
        else if (
            std::isprint(
                static_cast<unsigned char>(ch)))
        {
            {
                std::lock_guard<std::mutex> lock(
                    input_buffer_mutex);

                current_input_buffer += ch;
            }

            draw_command_prompt();
        }
    }
}


// Processes commands entered by the user
void command_handler(
    const std::string& command_line)
{
    std::string output;

    // Display available commands
    if (command_line == "help")
    {
        output =
            "\nAvailable commands:\n"
            " help           - Show commands\n"
            " start_marquee  - Start marquee animation\n"
            " stop_marquee   - Stop marquee animation\n"
            " set_text       - Set new text\n"
            " set_speed      - Set marquee speed in milliseconds\n"
            " exit           - Terminate the console\n\n";
    }

    // Start the marquee animation
    else if (command_line == "start_marquee")
    {
        bool has_text;

        {
            std::lock_guard<std::mutex> lock(
                marquee_text_mutex);

            has_text =
                !marquee_text.empty();
        }

        if (!has_text)
        {
            output =
                "The text field is empty. "
                "Please use 'set_text' first.\n";
        }
        else if (marquee_running)
        {
            output =
                "Marquee is already running.\n";
        }
        else
        {
            marquee_running = true;

            marquee_thread =
                std::thread(
                    marquee_animation);

            output = "";
        }
    }

    // Stop the marquee animation
    else if (command_line == "stop_marquee")
    {
        if (marquee_running)
        {
            marquee_running = false;

            if (marquee_thread.joinable())
            {
                marquee_thread.join();
            }

            output =
                "\nMarquee has stopped.\n";
        }
        else
        {
            output =
                "Marquee isn't running right now.\n";
        }
    }

    // Handle set_text without a value
    else if (command_line == "set_text")
    {
        output =
            "Please provide text.\n"
            "Format: set_text <your text>\n";
    }

    // Set the marquee text
    else if (
        command_line.rfind(
            "set_text ", 0) == 0)
    {
        std::string new_text =
            command_line.substr(9);

        if (new_text.empty())
        {
            output =
                "Please provide text.\n"
                "Format: set_text <your text>\n";
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock(
                    marquee_text_mutex);

                marquee_text = new_text;
            }

            output =
                "Text has been updated.\n";
        }
    }

    // Handle set_speed without a value
    else if (command_line == "set_speed")
    {
        output =
            "Please provide speed.\n"
            "Format: set_speed <milliseconds>\n";
    }

    // Change the marquee speed
    else if (
        command_line.rfind(
            "set_speed ", 0) == 0)
    {
        std::string speed_text =
            command_line.substr(10);

        if (speed_text.empty())
        {
            output =
                "Please provide speed.\n"
                "Format: set_speed <milliseconds>\n";
        }
        else
        {
            try
            {
                size_t position = 0;

                int speed =
                    std::stoi(
                        speed_text,
                        &position);

                // Check for extra characters in the input
                if (position !=
                    speed_text.length())
                {
                    output =
                        "Invalid Speed. "
                        "Try Again.\n";
                }

                // Speed must be greater than zero
                else if (speed <= 0)
                {
                    output =
                        "Invalid Speed. "
                        "Speed must be greater than 0.\n";
                }
                else
                {
                    {
                        std::lock_guard<std::mutex> lock(
                            marquee_speed_mutex);

                        marquee_speed = speed;
                    }

                    output =
                        "Speed set to " +
                        std::to_string(speed) +
                        " ms.\n";
                }
            }
            catch (...)
            {
                output =
                    "Invalid Speed. "
                    "Try Again.\n";
            }
        }
    }

    // Exit the program
    else if (command_line == "exit")
    {
        marquee_running = false;
        is_running = false;

        if (marquee_thread.joinable())
        {
            marquee_thread.join();
        }

        output =
            "\nSession Ending...\n";
    }

    // Handle commands that are not recognized
    else
    {
        output =
            "Unknown command. "
            "Type 'help' for available commands.\n";
    }


    // Display the result and restore the prompt
    {
        std::lock_guard<std::mutex> lock(
            console_mutex);

        if (marquee_running)
        {
            // Clear the current prompt
            std::cout
                << "\033[2K\r";

            // Print the command result on a new line
            std::cout
                << '\n'
                << output;

            // Restore the prompt below the output
            std::cout
                << "Command> ";

            std::cout.flush();
        }
        else
        {
            // Clear the current line
            std::cout
                << "\033[2K\r";

            // Print the command result
            std::cout
                << output;

            if (is_running)
            {
                std::cout
                    << "Command> ";
            }

            std::cout.flush();
        }
    }
}


// Takes commands from the queue and processes them
void command_interpreter_thread_func()
{
    while (is_running)
    {
        std::string command;

        {
            std::lock_guard<std::mutex> lock(
                command_queue_mutex);

            if (!command_queue.empty())
            {
                command =
                    command_queue.front();

                command_queue.pop();
            }
        }

        if (!command.empty())
        {
            command_handler(command);
        }

        // Prevent the thread from constantly checking the queue
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }
}


int main()
{
    // Initial console display
    std::cout
        << "=====================================\n"
        << "      CSOPESY - M03 MARQUEE\n"
        << "      S05 - GROUP 8\n"
        << "=====================================\n\n";

    std::cout
        << "Group Developer:\n\n"
        << "Abenojar Fredrikzen\n"
        << "Caya, Mary Faye\n"
        << "Diamante, Deo Zamir\n"
        << "Guiller, Gerylyn\n\n\n";

    std::cout
        << "Version Date: September 26, 2026\n\n";

    std::cout
        << "Type 'help' to see available commands.\n\n";

    std::cout
        << "Command> ";

    std::cout.flush();


    // Start the input and command threads
    std::thread keyboard_thread(
        keyboard_handler_thread_func);

    std::thread command_thread(
        command_interpreter_thread_func);


    // Wait until the command thread finishes
    command_thread.join();


    // Stop the marquee before exiting
    marquee_running = false;

    if (marquee_thread.joinable())
    {
        marquee_thread.join();
    }


    // The keyboard thread is waiting for input,
    // so it is detached when the program ends.
    keyboard_thread.detach();


    std::cout
        << "\nProgram terminated.\n";

    return 0;
}
