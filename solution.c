/*
 * Author: Jason Housewright
 * Course: INET 3101
 * Assignment: Coding Assignment 4
 * File: solution.c
 *
 * Purpose:
 *   Extend the Colossus Airlines reservation program so that
 *   Outbound and Inbound seat assignments persist between runs.
 *
 * Features:
 *   - Manage two flights with 24 seats each.
 *   - Count and list empty seats.
 *   - Display assigned seats alphabetically.
 *   - Assign and delete reservations with cancellation support.
 *   - Load saved flight data automatically at startup.
 *   - Save both flights when Quit is selected.
 *   - Validate file contents and reject corrupted data safely.
 *
 * File Design:
 *   flight_data.bin contains the eight-byte header COLFLT01,
 *   followed by 24 Outbound and 24 Inbound seat structures.
 *   Records are loaded into temporary arrays and validated
 *   before being copied into the active flight arrays.
 *
 * Error Handling:
 *   A missing file starts both flights with empty seats.
 *   Invalid or unreadable data stops startup without changing
 *   the existing file. Saving uses a temporary file, and a save
 *   failure returns to the menu with reservations still in memory.
 *
 * Limitations:
 *   Binary structure storage depends on the compiler and platform.
 *   Changes are saved through the main menu's Quit option;
 *   forced termination or end-of-input does not save changes.
 *
 * AI Assistance:
 *   OpenAI Codex assisted with implementation, explanations,
 *   debugging, and creation of the Python corruption tests.
 *   I compiled and ran the program in VS Code and checked
 *   persistence, corrupted-file rejection, and save-failure recovery.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define SEAT_COUNT 24
#define NAME_LENGTH 50
#define DATA_FILE "flight_data.bin"
#define FILE_HEADER "COLFLT01"
#define TEMP_FILE "flight_data.bin.tmp"

struct seat {
    int seat_id;
    int assigned;
    char last_name[NAME_LENGTH];
    char first_name[NAME_LENGTH];
};

void initialize_seats(struct seat seats[], int count);
int read_menu_choice(void);
void flight_menu(struct seat seats[], int count, const char flight_name[]);
int count_empty_seats(const struct seat seats[], int count);
void list_empty_seats(const struct seat seats[], int count);
int read_text(const char prompt[], char text[], int size);
void assign_seat(struct seat seats[], int count);
void delete_assignment(struct seat seats[], int count);
int compare_names(const char first[], const char second[]);
void list_alphabetical(const struct seat seats[], int count);
int save_flights(const struct seat outbound[],
                 const struct seat inbound[]);
int valid_saved_name(const char name[], int assigned);
int validate_flight(const struct seat seats[], const char flight_name[]);
int load_flights(struct seat outbound[], struct seat inbound[]);
int read_passenger_name(const char prompt[], char name[], int size);

int main(void)
{
    struct seat outbound[SEAT_COUNT];
    struct seat inbound[SEAT_COUNT];
    int choice;

    initialize_seats(outbound, SEAT_COUNT);
    initialize_seats(inbound, SEAT_COUNT);

    if (!load_flights(outbound, inbound)) {
        fprintf(stderr,
                "Startup stopped. The existing data file was not changed.\n");
        return EXIT_FAILURE;
    }

    while (1) {
        printf("\nColossus Airlines\n");
        printf("a) Outbound Flight\n");
        printf("b) Inbound Flight\n");
        printf("c) Quit\n");
        printf("Enter your choice: ");
        fflush(stdout);

        choice = read_menu_choice();

        if (choice == EOF) {
            printf("\nEnd of input. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 'a':
                flight_menu(outbound, SEAT_COUNT, "Outbound Flight");
                break;

            case 'b':
                flight_menu(inbound, SEAT_COUNT, "Inbound Flight");
                break;

            case 'c':
                if (save_flights(outbound, inbound)) {
                    printf("Goodbye!\n");
                    return EXIT_SUCCESS;
                }

                printf("Save failed. Your reservations are still in memory.\n");
                printf("Returning to the main menu so you can retry.\n");
                break;

            default:
                printf("Invalid selection. Enter a, b, or c.\n");
                break;
        }
    }

    return 0;
}

void initialize_seats(struct seat seats[], int count)
{
    memset(seats, 0, sizeof(struct seat) * count);

    for (int i = 0; i < count; i++) {
        seats[i].seat_id = i + 1;
    }
}

int read_menu_choice(void)
{
    int choice = getchar();
    int extra;
    int has_extra = 0;

    if (choice == EOF || choice == '\n') {
        return choice;
    }

    while ((extra = getchar()) != '\n' && extra != EOF) {
        has_extra = 1;
    }

    if (has_extra) {
        return '\0';
    }

    return choice;
}

void flight_menu(struct seat seats[], int count, const char flight_name[])
{
    int choice;

    while (1) {
        printf("\n%s\n", flight_name);
        printf("a) Show number of empty seats\n");
        printf("b) Show list of empty seats\n");
        printf("c) Show alphabetical list of seats\n");
        printf("d) Assign a customer to a seat\n");
        printf("e) Delete a seat assignment\n");
        printf("f) Return to Main Menu\n");
        printf("Enter your choice: ");
        fflush(stdout);

        choice = read_menu_choice();

        if (choice == EOF) {
            return;
        }

        switch (choice) {
            case 'a':
                printf("Number of empty seats: %d\n",
                       count_empty_seats(seats, count));
                break;

            case 'b':
                list_empty_seats(seats, count);
                break;

            case 'd':
                assign_seat(seats, count);
                break;

            case 'e':
                delete_assignment(seats, count);
                break;

            case 'c':
                list_alphabetical(seats, count);
                break;

            case 'f':
                return;

            default:
                printf("Invalid selection. Enter a through f.\n");
                break;
        }
    }
}

int count_empty_seats(const struct seat seats[], int count)
{
    int empty_count = 0;

    for (int i = 0; i < count; i++) {
        if (seats[i].assigned == 0) {
            empty_count++;
        }
    }

    return empty_count;
}

void list_empty_seats(const struct seat seats[], int count)
{
    if (count_empty_seats(seats, count) == 0) {
        printf("No empty seats available.\n");
        return;
    }

    printf("Empty seat IDs:");

    for (int i = 0; i < count; i++) {
        if (seats[i].assigned == 0) {
            printf(" %d", seats[i].seat_id);
        }
    }

    printf("\n");
}

int read_text(const char prompt[], char text[], int size)
{
    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        if (fgets(text, size, stdin) == NULL) {
            return 0;
        }

        char *newline = strchr(text, '\n');

        if (newline != NULL) {
            *newline = '\0';
        } else {
            int extra = getchar();

            if (extra != '\n' && extra != EOF) {
                while ((extra = getchar()) != '\n' && extra != EOF) {
                    /* Discard the remainder of an oversized line. */
                }

                printf("Input too long. Maximum: %d characters.\n",
                       size - 1);
                continue;
            }
        }

        if (strcmp(text, "/cancel") == 0) {
            return 0;
        }

        if (text[strspn(text, " \t\r")] == '\0') {
            printf("Input cannot be blank.\n");
            continue;
        }

        return 1;
    }
}

void assign_seat(struct seat seats[], int count)
{
    char input[50];
    char first_name[NAME_LENGTH];
    char last_name[NAME_LENGTH];
    char *end;
    long seat_id;
    int index;

    if (count_empty_seats(seats, count) == 0) {
        printf("This flight is full.\n");
        return;
    }

    printf("Enter /cancel at any prompt to cancel the assignment.\n");
    list_empty_seats(seats, count);

    while (1) {
        if (!read_text("Enter seat ID: ", input, sizeof input)) {
            printf("Assignment cancelled.\n");
            return;
        }

        errno = 0;
        seat_id = strtol(input, &end, 10);

        if (errno == ERANGE || end == input || *end != '\0' ||
            seat_id < 1 || seat_id > count) {
            printf("Enter a whole number from 1 to %d.\n", count);
            continue;
        }

        index = (int)seat_id - 1;

        if (seats[index].assigned == 1) {
            printf("That seat is already assigned. Choose another seat.\n");
            continue;
        }

        break;
    }

    if (!read_passenger_name("Enter first name: ",
                             first_name, sizeof first_name)) {
        printf("Assignment cancelled.\n");
        return;
    }

    if (!read_passenger_name("Enter last name: ",
                             last_name, sizeof last_name)) {
        printf("Assignment cancelled.\n");
        return;
    }

    strcpy(seats[index].first_name, first_name);
    strcpy(seats[index].last_name, last_name);
    seats[index].assigned = 1;

    printf("Seat %d assigned to %s %s.\n",
           seats[index].seat_id,
           seats[index].first_name,
           seats[index].last_name);
}

void delete_assignment(struct seat seats[], int count)
{
    char input[50];
    char confirmation[20];
    char *end;
    long seat_id;
    int index;

    printf("Enter /cancel at any prompt to cancel deletion.\n");

    while (1) {
        if (!read_text("Enter seat ID to delete: ", input, sizeof input)) {
            printf("Deletion cancelled.\n");
            return;
        }

        errno = 0;
        seat_id = strtol(input, &end, 10);

        if (errno == ERANGE || end == input || *end != '\0' ||
            seat_id < 1 || seat_id > count) {
            printf("Enter a whole number from 1 to %d.\n", count);
            continue;
        }

        index = (int)seat_id - 1;

        if (seats[index].assigned == 0) {
            printf("That seat is already empty. Choose another seat.\n");
            continue;
        }

        break;
    }

    printf("Seat %d is assigned to %s %s.\n",
           seats[index].seat_id,
           seats[index].first_name,
           seats[index].last_name);

    while (1) {
        if (!read_text("Delete this assignment? (y/n): ",
                       confirmation, sizeof confirmation)) {
            printf("Deletion cancelled.\n");
            return;
        }

        if (strcmp(confirmation, "n") == 0) {
            printf("Deletion cancelled.\n");
            return;
        }

        if (strcmp(confirmation, "y") == 0) {
            break;
        }

        printf("Enter y, n, or /cancel.\n");
    }

    seats[index].assigned = 0;
    seats[index].first_name[0] = '\0';
    seats[index].last_name[0] = '\0';

    printf("Assignment deleted. Seat %d is now empty.\n",
           seats[index].seat_id);
}

int compare_names(const char first[], const char second[])
{
    while (*first != '\0' && *second != '\0') {
        int a = tolower((unsigned char)*first);
        int b = tolower((unsigned char)*second);

        if (a != b) {
            return a - b;
        }

        first++;
        second++;
    }

    return tolower((unsigned char)*first) -
           tolower((unsigned char)*second);
}

void list_alphabetical(const struct seat seats[], int count)
{
    struct seat sorted[SEAT_COUNT];
    int assigned_count = 0;

    for (int i = 0; i < count; i++) {
        if (seats[i].assigned == 1) {
            sorted[assigned_count] = seats[i];
            assigned_count++;
        }
    }

    if (assigned_count == 0) {
        printf("No assigned seats to display.\n");
        return;
    }

    /* Insertion sort: place each record in alphabetical order. */
    for (int i = 1; i < assigned_count; i++) {
        struct seat current = sorted[i];
        int j = i - 1;

        while (j >= 0) {
            int comparison = compare_names(
                sorted[j].last_name, current.last_name);

            if (comparison == 0) {
                comparison = compare_names(
                    sorted[j].first_name, current.first_name);
            }

            if (comparison <= 0) {
                break;
            }

            sorted[j + 1] = sorted[j];
            j--;
        }

        sorted[j + 1] = current;
    }

    printf("Assigned seats in alphabetical order:\n");

    for (int i = 0; i < assigned_count; i++) {
        printf("Seat %d: %s, %s\n",
               sorted[i].seat_id,
               sorted[i].last_name,
               sorted[i].first_name);
    }
}

int save_flights(const struct seat outbound[],
                 const struct seat inbound[])
{
    FILE *file = fopen(TEMP_FILE, "wb");
    int success = 1;

    if (file == NULL) {
        perror("Could not open temporary save file");
        return 0;
    }

    if (fwrite(FILE_HEADER, 1, sizeof(FILE_HEADER) - 1, file)
            != sizeof(FILE_HEADER) - 1) {
        fprintf(stderr, "Could not write the file header.\n");
        success = 0;
    }

    if (success &&
        fwrite(outbound, sizeof(struct seat), SEAT_COUNT, file)
            != SEAT_COUNT) {
        fprintf(stderr, "Could not write Outbound flight records.\n");
        success = 0;
    }

    if (success &&
        fwrite(inbound, sizeof(struct seat), SEAT_COUNT, file)
            != SEAT_COUNT) {
        fprintf(stderr, "Could not write Inbound flight records.\n");
        success = 0;
    }

    if (fflush(file) == EOF) {
        perror("Could not flush saved data");
        success = 0;
    }

    if (fclose(file) == EOF) {
        perror("Could not close the save file");
        success = 0;
    }

    if (!success) {
        return 0;
    }

    if (rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Could not replace the data file");
        return 0;
    }

    printf("Both flights saved to %s.\n", DATA_FILE);
    return 1;
}

int valid_saved_name(const char name[], int assigned)
{
    const char *terminator = memchr(name, '\0', NAME_LENGTH);

    if (terminator == NULL) {
        return 0;
    }

    if (assigned == 0) {
        return name[0] == '\0';
    }

    int has_nonspace = 0;

    for (const char *p = name; p < terminator; p++) {
        unsigned char character = (unsigned char)*p;

        if (!isprint(character)) {
            return 0;
        }

        if (!isspace(character)) {
            has_nonspace = 1;
        }
    }

    return has_nonspace;
}

int validate_flight(const struct seat seats[], const char flight_name[])
{
    for (int i = 0; i < SEAT_COUNT; i++) {
        if (seats[i].seat_id != i + 1) {
            fprintf(stderr,
                    "%s: invalid seat ID at record %d.\n",
                    flight_name, i + 1);
            return 0;
        }

        if (seats[i].assigned != 0 && seats[i].assigned != 1) {
            fprintf(stderr,
                    "%s: invalid assignment status at seat %d.\n",
                    flight_name, i + 1);
            return 0;
        }

        if (!valid_saved_name(seats[i].first_name, seats[i].assigned) ||
            !valid_saved_name(seats[i].last_name, seats[i].assigned)) {
            fprintf(stderr,
                    "%s: invalid passenger name at seat %d.\n",
                    flight_name, i + 1);
            return 0;
        }
    }

    return 1;
}

int load_flights(struct seat outbound[], struct seat inbound[])
{
    FILE *file = fopen(DATA_FILE, "rb");
    struct seat loaded_outbound[SEAT_COUNT];
    struct seat loaded_inbound[SEAT_COUNT];
    char header[sizeof(FILE_HEADER) - 1];
    int success = 1;

    if (file == NULL) {
        if (errno == ENOENT) {
            printf("No saved data found. Starting with empty flights.\n");
            return 1;
        }

        perror("Could not open saved flight data");
        return 0;
    }

    if (fread(header, 1, sizeof header, file) != sizeof header) {
        fprintf(stderr, "Could not read the complete file header.\n");
        success = 0;
    } else if (memcmp(header, FILE_HEADER, sizeof header) != 0) {
        fprintf(stderr, "Invalid file header or unsupported version.\n");
        success = 0;
    }

    if (success &&
        fread(loaded_outbound, sizeof(struct seat), SEAT_COUNT, file)
            != SEAT_COUNT) {
        fprintf(stderr, "Incomplete Outbound flight data.\n");
        success = 0;
    }

    if (success &&
        fread(loaded_inbound, sizeof(struct seat), SEAT_COUNT, file)
            != SEAT_COUNT) {
        fprintf(stderr, "Incomplete Inbound flight data.\n");
        success = 0;
    }

    if (success) {
        int extra = fgetc(file);

        if (extra != EOF) {
            fprintf(stderr, "Unexpected extra bytes after flight records.\n");
            success = 0;
        }
    }

    if (ferror(file)) {
        fprintf(stderr, "An error occurred while reading the data file.\n");
        success = 0;
    }

    if (fclose(file) == EOF) {
        perror("Could not close the data file");
        success = 0;
    }

    if (!success) {
        return 0;
    }

    if (!validate_flight(loaded_outbound, "Outbound Flight") ||
        !validate_flight(loaded_inbound, "Inbound Flight")) {
        return 0;
    }

    memcpy(outbound, loaded_outbound, sizeof loaded_outbound);
    memcpy(inbound, loaded_inbound, sizeof loaded_inbound);

    printf("Both flights loaded from %s.\n", DATA_FILE);
    return 1;
}

int read_passenger_name(const char prompt[], char name[], int size)
{
    while (1) {
        if (!read_text(prompt, name, size)) {
            return 0;
        }

        int valid = 1;

        for (int i = 0; name[i] != '\0'; i++) {
            if (!isprint((unsigned char)name[i])) {
                valid = 0;
                break;
            }
        }

        if (valid) {
            return 1;
        }

        printf("Use printable letters, spaces, or punctuation only.\n");
    }
}