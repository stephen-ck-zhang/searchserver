# searchserver

The project implements a multi-threaded web server that provides simple searching and file-viewing utilities in C++.

_This is the project completed for Penn CIT 595 Computer Systems Programming course, Spring 2024. All rights reserved._


### How to run the project

1. Clone the repository to your local machine.
2. Navigate to the project directory.
3. Register a New Port in the Docker VM environment and install the Boost library.
4. To start the project, run the following command in the terminal:
   ```
    make
    ./httpd 5950 ./test_tree/
    ```
5. The project will be running on `http://localhost:5950/`.
