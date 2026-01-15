# cpp-texted

### What is cpp-texted?

**cpp-texted** is a lightweight and modular text editor built in C++. This project was created as an **educational exploration** aimed at developing custom UI layout widgets and practicing modern C++ programming techniques. The application doubles as a functional text editor and a testbed for experimenting with UI design and interactions.

### Key Features

- **Text Editing:** Simple text editor with basic file management (open, save, save as).
- **Custom UI Framework:**
  - Built from scratch to provide a deep dive into UI layout logic.
  - Widgets include buttons, labels, input fields, and layout containers like `VerticalBox` and `HorizontalBox`.
- **Lexer for Syntax Highlighting:** A small C++-oriented lexer for analyzing and highlighting keywords, literals, and comments.
- **File Management:** Allows opening and saving text files with an indicator for unsaved changes.
- **Lightweight & Modular:** Focused on clean and efficient design with minimal dependencies.

### Why This Project?

The primary goal of **cpp-texted** is to:
- Learn about creating reusable components in a graphical UI framework.
- Experiment with designing and implementing widgets and layout managers.
- Build a deeper understanding of systems like event handling, drawing routines, and interactive elements.

### Technical Highlights

- **Lexer (`Lexer.cpp`/`Lexer.h`):**
  - Converts source code into tokens for syntax highlighting.
  - Recognizes various elements like keywords, operators, and comments.
- **Widgets (`Widgets.cpp`/`Widgets.h`):**
  - Includes basic UI elements such as buttons and input fields.
  - Layout management using flexible vertical and horizontal box systems.
- **Main Application (`main.cpp`):**
  - Implements the window layout and user interactions.
  - Handles core text editor tasks like file loading, saving, and live text updates.

### Getting Started

To explore and try out the project:

1. Clone the repo:
   ```bash
   git clone https://github.com/Butjok/cpp-texted.git
   cd cpp-texted
   ```

2. Build the application using CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Run the application:
   ```bash
   ./cpp-texted
   ```

### What's Next?

Some potential directions for this project might include:
- Extending the UI framework with additional widgets (e.g., dropdowns, modals).
- Improving syntax highlighting and expanding it to support more languages.
- Adding features like undo/redo and find/replace for the text editor.
- Introducing themes and color customization for the editor.

---

**cpp-texted** remains a work-in-progress but represents a fantastic learning journey into C++ and UI design. Contributions, feedback, and ideas are always welcome! 🚀
