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

#include <iostream> 
#include <string> 
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <cctype>

// ============================================================
//                         SHARED STATE
// ============================================================

std::atomic<bool> is_running{true};

std::atomic<bool> marquee_running{false};

std::string marquee_text;
std::mutex marquee_text_mutex;

int marquee_speed = 100;
std::mutex marquee_speed_mutex;

std::queue<std::string> command_queue;
std::mutex command_queue_mutex;

std::string current_input_buffer;
std::mutex input_buffer_mutex;

std::string prompt_display_buffer;
std::mutex prompt_mutex;


// ============================================================
//                  KEYBOARD / INPUT SYSTEM
// ============================================================

void keyboard_handler_thread_func()
{
    while (is_running)
    {
        char ch = std::cin.get();

        if (ch == '\n')
        {
            std::string command;

            {
                std::lock_guard<std::mutex> input_lock(
                    input_buffer_mutex
                );

                command = current_input_buffer;
                current_input_buffer.clear();
            }

            if (!command.empty())
            {
                std::lock_guard<std::mutex> queue_lock(
                    command_queue_mutex
                );

                command_queue.push(command);
            }
        }

        else if (ch == '\b' || ch == 127)
        {
            std::lock_guard<std::mutex> input_lock(
                input_buffer_mutex
            );

            if (!current_input_buffer.empty())
            {
                current_input_buffer.pop_back();
            }
        }

        else if (
            std::isprint(
                static_cast<unsigned char>(ch)
            )
        )
        {
            std::lock_guard<std::mutex> input_lock(
                input_buffer_mutex
            );

            current_input_buffer += ch;
        }
    }
}


// ============================================================
//                     COMMAND HANDLER
// ============================================================

void command_handler(
    const std::string& command_line
)
{
    std::string output;


    // ========================================================
    // HELP
    // ========================================================

    if (command_line == "help")
    {
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

    else if (command_line == "start_marquee")
    {
        bool has_text;

        {
            std::lock_guard<std::mutex> lock(
                marquee_text_mutex
            );

            has_text = !marquee_text.empty();
        }

        if (!has_text)
        {
            output =
                "The text field is empty. "
                "Please use 'set_text' first.\n";
        }
        else
        {
            output =
                "Marquee is ready.\n"
                "Stored text: [" +
                marquee_text +
                "]\n";
        }
    }


    // ========================================================
    // STOP MARQUEE - UPDATE THIS
    // ========================================================

    else if (command_line == "stop_marquee")
    {
        if (marquee_running)
        {
            marquee_running = false;

            output =
                "Marquee has stopped.\n";
        }
        else
        {
            output =
                "Marquee isn't running right now.\n";
        }
    }


    // ========================================================
    // SET TEXT - UPDATE THIS
    // ========================================================

    else if (command_line == "set_text")
    {
        output =
            "Please provide text.\n"
            "Format: set_text <your text>\n";
    }


    else if (
        command_line.rfind("set_text ", 0) == 0
    )
    {
        // ----------------------------------------------------
        // Get text after "set_text "
        // ----------------------------------------------------

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
            // ------------------------------------------------
            // Store text
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex> lock(
                    marquee_text_mutex
                );

                marquee_text = new_text;
            }


            // ------------------------------------------------
            // REMOVE THIS PART AFTER
            // Temporary display to verify that the input was successfully stored.
            // ------------------------------------------------

            output =
                "Text has been updated.\n"
                "Current text: [" +
                new_text +
                "]\n";
        }
    }


    // ========================================================
    // SET SPEED - UPDATE THIS
    // ========================================================

    else if (command_line == "set_speed")
    {
        output =
            "Please provide speed.\n"
            "Format: set_speed <milliseconds>\n";
    }


    // --------------------------------------------------------
    // SET SPEED WITH VALUE
    // --------------------------------------------------------

    else if (
        command_line.rfind("set_speed ", 0) == 0
    )
    {
        std::string speed_text =
            command_line.substr(10);


        // ----------------------------------------------------
        // Check if speed is empty
        // ----------------------------------------------------

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
                        &position
                    );


                // ------------------------------------------------
                // Check for extra characters
                // ------------------------------------------------

                if (position != speed_text.length())
                {
                    output =
                        "Unsupported Speed Value.\n";
                        "Format: set_speed <milliseconds>\n";
                }


                // ------------------------------------------------
                // Check for negative/zero value
                // ------------------------------------------------

                else if (speed <= 0)
                {
                    output =
                        "Unsupported Speed Value."
                        "Speed must be greater than 0.\n";
                }


                // ------------------------------------------------
                // Valid speed
                // ------------------------------------------------

                else
                {
                    
                    {
                        std::lock_guard<std::mutex> lock(
                            marquee_speed_mutex
                        );

                        marquee_speed = speed;
                    }

                    output =
                        "Speed set to "
                        + std::to_string(speed)
                        + " ms.\n";
                }
            }
            catch (...)
            {
                output =
                    "Unsupported Speed Value.\n";
                    "Format: set_speed <milliseconds>\n";
            }
        }
    }


    // ========================================================
    // EXIT
    // ========================================================

    else if (command_line == "exit")
    {
        marquee_running = false;
        is_running = false;

        output =
            "Session Ending...\n";
    }


    // ========================================================
    // UNKNOWN COMMAND
    // ========================================================

    else
    {
        output =
            "Unknown command. "
            "Type 'help' for available commands.\n";
    }


    // ========================================================
    // STORE OUTPUT
    // ========================================================

    {
        std::lock_guard<std::mutex> lock(
            prompt_mutex
        );

        prompt_display_buffer = output;
    }


    // ========================================================
    // DISPLAY OUTPUT
    // ========================================================

    std::cout << output;

    if (is_running)
    {
        std::cout << "Command> ";
        std::cout.flush();
    }
}


// ============================================================
//               COMMAND INTERPRETER THREAD
// ============================================================

void command_interpreter_thread_func()
{
    while (is_running)
    {
        std::string command;


        {
            std::lock_guard<std::mutex> lock(
                command_queue_mutex
            );


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


        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }
}


// ============================================================
//                            MAIN
// ============================================================

int main()
{
    std::cout
        << "=====================================\n";

    std::cout
        << "      CSOPESY - M03 MARQUEE\n";

    std::cout
        << "      S05 - GROUP 8\n";

    std::cout
        << "=====================================\n\n";


    std::cout
        << "Group Developer:\n\n";

    std::cout
        << "Member 1\n";

    std::cout
        << "Caya, Mary Faye\n";

    std::cout
        << "Member 2\n";

    std::cout
        << "Member 3\n\n\n";


    std::cout
        << "Version Date: September 23, 2026\n\n";


    std::cout
        << "Type 'help' to see available commands.";

    std::cout
        << "\n\n Command> ";

    std::cout.flush();

    std::thread keyboard_thread(
        keyboard_handler_thread_func
    );

    std::thread command_thread(
        command_interpreter_thread_func
    );

    command_thread.join();

    marquee_running = false;

    keyboard_thread.detach();


    std::cout
        << "\nProgram terminated.\n";


    return 0;
}