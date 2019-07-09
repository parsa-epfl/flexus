# Flexus Code Formatting #

The version of Ubuntu is 18.04 at least.
When you contribute to this project, please clang-format the code at first. Just follow instructions as follow:

`sudo apt-get install clang-format-6.0`

Use the `.clang-format` file from Repository and locate it in the folder where you want to format the code.

Within the folder, use this command to format .cpp/.hpp/.c/.h files:

`find . -name "*.hpp" -or -name "*.cpp" -or -name "*.h" -or -name "*.c" | xargs clang-format -i`

Use the `clang_format_test.sh` file to check whether the formatting is successful or not. If it is successful, there will be green works showing *All source code in commit are properly formatted*. Otherwise, there will be red warning showing *Found formatting errors!* and which file where the problem is in.
