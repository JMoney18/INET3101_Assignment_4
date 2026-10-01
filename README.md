# Coding Assignment 4: Colossus Airlines File Persistence

**Name:** Jason Housewright  
**Course:** INET 3101

## Problem Statement & Persistence Design

For this assignment, I extended my Assignment 3 reservation program so that seat assignments remain available after the program closes. The program still manages separate Outbound and Inbound flights with 24 seats each, but now loads saved reservations at startup and saves both flights when the user selects Quit.

I used a binary file named `flight_data.bin`. The file begins with the eight-byte header `COLFLT01`, followed by the 24 Outbound seat structures and then the 24 Inbound seat structures. On my Mac, each structure occupies 108 bytes, making the complete file 5,192 bytes.

At startup, the program initializes both flights with empty seats before attempting to load the file. If no file exists, it continues with empty flights. If a file exists, the program reads its contents into temporary arrays and validates them before copying the records into the active flight arrays. Invalid or unreadable files cause the program to report an error and stop without changing the existing file.

When saving, the program writes the header and both flight arrays to `flight_data.bin.tmp`. It checks the write, flush, and close operations before renaming the temporary file to `flight_data.bin`. This helps protect the previous saved data if writing fails. If saving is unsuccessful, the program returns to the main menu with the current reservations still in memory so the user can retry.

## File Validation & Error Recovery Analysis

The loading function checks the file header, the amount of data read, and the contents of each seat record before accepting saved reservations. The header must match `COLFLT01`. If it is incomplete or does not match, the program reports an error and stops loading.

Each `fread()` call must return the expected number of records. If the file ends before all 24 seats for each flight are read, the program rejects it as incomplete. After reading both flights, the loader checks for extra bytes to detect oversized files. It also checks for read errors and verifies that the file closes successfully.

The program validates each seat ID against its expected position and requires the assignment status to be either 0 or 1. Each passenger name must contain a null terminator within its 50-byte array. Assigned seats must have printable, nonblank names, while unassigned seats must have empty names. These checks prevent corrupted strings from reaching functions that expect properly terminated text.

Records are read into temporary arrays, so a partially valid file cannot partially replace the active flight data. If any check fails, the program exits with a failure status and leaves the existing file unchanged. It does not attempt to repair corrupted records automatically.

During development, I found an issue where the program loaded reservations and then cleared them by initializing the arrays afterward. Moving initialization before loading corrected the problem. A second run then successfully restored the saved passenger.

I also tested a save failure by creating a directory with the same name as the temporary save file. The program reported the error and returned to the menu while retaining the reservation in memory. After removing the blocking directory, I retried Quit, and the save completed successfully.

## Pros & Cons of Solution

Using binary structure storage made saving and loading straightforward because the program can write and read the flight arrays with `fwrite()` and `fread()`. The fixed layout also makes it easier to check the expected file length and reject incomplete or oversized files. A header identifies the format, and validation checks the loaded records before the program uses them.

One disadvantage is portability. Structure sizes, padding, integer representation, and byte order can differ between compilers and computers. A file created on my Mac may not load correctly on another platform. Changes to the seat structure would also require updating the file format and loading logic.

Binary files are harder to inspect manually than formatted text or CSV files. A text format would be easier to read and generally more portable, but it would require parsing numbers, handling delimiters, and validating each field. CSV would also need proper quoting for names containing commas or quotation marks.

Saving through a temporary file helps preserve the previous data if writing fails. However, the program saves only when the user selects Quit. Unexpected termination or end-of-input can therefore lose changes made during the current run. The current validation also cannot detect every possible corruption: a changed name that still contains valid printable characters could pass the checks.

## AI Fuzzing Reflection

I worked with ChatGPT throughout development to add file persistence, understand validation checks, and create `fuzzer.py`. The script generates a valid reference file and 11 deliberately corrupted files. Each case is stored in a separate folder so testing does not overwrite my main reservation file.

The corrupted cases include empty files, incomplete or incorrect headers, truncated flight records, extra bytes, invalid seat IDs, invalid assignment flags, and damaged passenger names. Name tests include nonprintable characters, missing null terminators, and blank names on assigned seats.

The script runs the C program against each file and checks the results. The valid case must load the expected passengers, display the correct seat counts, and save successfully. Corrupted cases must produce the expected error, exit with status 1, and leave the tested file unchanged. A five-second timeout also helps detect a program that becomes stuck.

All 12 cases passed. The corrupted files did not cause crashes or require additional changes to the loading logic during this test. Working with AI during implementation helped me include validation before running the fuzzer. The initialization-order bug was found and fixed during the earlier persistence test, rather than by the corrupted files.

These tests cover specific corruption scenarios rather than every possible damaged file. They demonstrate that the program handled the tested cases correctly, but do not guarantee that all corruption will be detected.

### The Main Prompts I Used

- Help extend my Assignment 3 program to save both flights on quit and load them at startup.
- Explain how to validate the file header, detect incomplete or oversized files, and check loaded seat records.
- Help identify why saved reservations disappear after the program reports that they loaded successfully.
- Generate a Python script that creates corrupted files matching the assignment’s required test cases.
- Extend the script to check expected errors, exit statuses, and whether rejected files remain unchanged.
- Help test a save failure and confirm that reservations remain available when saving is retried.