````instructions
applyTo:

* "**/*.c"
* "**/*.h"

---

# 0. Development Environment

* The project is developed on **Windows 11** using **Visual Studio Code (VS Code)** as the primary editor.

# 1. Project Foundation

* This project is based on the open-source Azure RTOS and its middleware.
* Only use functions, macros, and variables from the Azure RTOS API that are actually present in the project codebase or official Azure RTOS sources.
* Do not invent, assume, or use undocumented or non-existent API elements. All usage of Azure RTOS API must be verifiable in the project or official documentation.

# 2. Formatting and Indentation

* Use **2 spaces** for indentation, **not** tabs.
* Use **Allman style** braces:

  ```c
  if (condition)
  {
      doSomething();
  }
  else
  {
      doSomethingElse();
  }
  ```
 * **Strictly avoid ternary operators** (`?:`) in all code. Use explicit if-else statements for all conditional assignments and logic.
   Any use of ternary operators is prohibited and will be considered a style violation.

# 3. Naming Conventions

* **Variables**: Use `snake_case` style (lowercase with underscores)
* **Global variables**: Use `snake_case` style (lowercase with underscores), names must start with "g_" prefix
* **Function names**: Use `snake_case` style but the first letter must **always** be capitalized, all others lowercase with underscores
* **Static function names**: Must **always** start with an underscore "_". Use `snake_case` style but the first letter after the underscore must **always** be capitalized, all others lowercase with underscores
* **Macros and constants**: Use `UPPER_SNAKE_CASE`
* **Types**: Must start with T_ prefix, e.g., T_sens_data, use `snake_case` style (lowercase with underscores)
* **Header files**: Use `module_name.h`, source files — `module_name.c`

# 4. Code Organization

* **Header files** should contain only declarations, macros, and structures.
* Always use **header guards**:

  ```c
  #ifndef MODULE_NAME_H
  #define MODULE_NAME_H
  ...
  #endif // MODULE_NAME_H
  ```
* **Static** (`static`) functions and variables intended for use only within the module must be marked as `static`.
* **If you implement a new function:**
  - If it is static, you must declare it at the top of the corresponding .c file.
  - If it is global, you must declare it in the header file. The header file name must match the .c file name, differing only by extension.
* **All function declarations (prototypes) and static helper functions must be placed at the top of the corresponding .c file, before any function that uses them. This avoids implicit declaration warnings or errors.**
* **Do not include header files in .h files** if they are already included in the main project header (usually named App.h).
  When creating new header files, do not add #include statements for other headers; all required includes must be present in App.h.
  The main header file can be identified by checking which header is included in all other project files.
* All memory allocation sizes that are known at compile time must be defined as macros at the top of the file. Do not use hardcoded numbers for allocation sizes in the code body if the size is known at compile time.

# 4.1 Header File Function Declarations

* Do not comment function declarations (prototypes) in header files. All documentation for global functions must be provided only in the corresponding .c file using the required block header format. Header files should contain only bare function prototypes, macros, and structures, without any comments for function declarations.

# 4.2 Header Inclusion Strictness

* It is strictly prohibited to add or include any header files in a module's .c or .h file except for the main project header (usually named App.h), unless explicitly required for toolchain/compiler attributes or unless the header is not already included in App.h. All additional includes must be justified and minimized. Any unnecessary or duplicate #include statements must be removed. This rule applies to both .c and .h files.
* When creating new header files, do not add #include statements for other headers; all required includes must be present in App.h. The only exception is for toolchain/compiler-specific headers required for memory attributes or section placement, which must be preserved as in the original code.

# 5. Memory Management

* For local data structures and arrays exceeding 200 bytes, **always** use dynamic memory allocation with App_malloc and App_free functions.
* For local data structures exceeding 10 kilobytes, use memory allocation with SDRAM_malloc and SDRAM_free functions.

# 6. Data Types

* Use **fixed-width types** from `<stdint.h>`: `uint8_t`, `int16_t`, `uint32_t`, etc.
* For flags, use `bool` from `<stdbool.h>`.

# 7. Documentation (Code Formatting)

* All comments and debug/log messages in the code **must be written in English only**. Any use of other languages in code comments or debug output is strictly prohibited.
* All code comments and debug/log messages are considered part of code formatting and must follow these rules.
* Write comments in English
* Always comment structure fields
* Use single-line comments // instead of multi-line comments /* ... */. **Exception for function headers**
* Comment statements in functions, but only if they are not trivial operations
* Insert comments after statements, not above them. Try to align comments within a single function
* All functions must begin with a block header in the following format (see example)
- The separator lines (-) must be of equal length.
- The first line inside the comment is a description of what the function does.
- The middle section describes each parameter.
- The last section describes the return value.
* **Do not comment function declarations (prototypes) in header (.h) files.**
* If you encounter function headers (block comments before functions) that do not match the required format shown below, you must rewrite them to match this format exactly.
* If you encounter comments that do not correspond to the actual code, you must update those comments so that they accurately describe the code's behavior.

Example:
```c
/*-----------------------------------------------------------------------------------------------------
  Description:

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
return_type function_name(type1 param1, type2 param2)
{
    // ...
}
```
* If you encounter function headers (block comments before functions) that do not match the required format shown below, you must rewrite them to match this format exactly.

# 8. Safety and Reliability

* Avoid **recursion**.
* When necessary, **break down** into helper functions.
* Avoid creating local variables larger than 256 bytes in the stack.

# 9. Toolchain/Compiler-Specific Attributes

* Do not remove or alter memory qualifiers, section attributes, or other toolchain/compiler-specific keywords (such as `@ ".sdram"`, `__attribute__`, `#pragma location`, etc.) in variable or function declarations. These are required for correct placement in memory or for other toolchain-specific behavior. Any such attribute must be preserved exactly as in the original code unless an explicit user request is made to change it.

---

*Follow these guidelines to improve readability, reliability, and portability of your code.*
