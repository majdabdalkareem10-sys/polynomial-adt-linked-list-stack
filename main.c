#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_FILE_NAME 260

/*
 * Polynomial ADT using a linked list of terms.
 * Multiple polynomials are stored in a stack.
 *
 * Author: Majd Alian
 * Course: COMP2421 - Data Structures
 */

typedef struct Term {
    long long coefficient;
    int exponent;
    struct Term *next;
} Term;

typedef struct StackNode {
    Term *polynomial;
    struct StackNode *next;
} StackNode;

typedef enum {
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY
} Operation;

/* ---------------------------- Utility functions ---------------------------- */

static void *xmalloc(size_t size) {
    void *memory = malloc(size);

    if (memory == NULL) {
        fprintf(stderr, "Fatal error: not enough memory.\n");
        exit(EXIT_FAILURE);
    }

    return memory;
}

static void discard_remaining_input(void) {
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* Discard characters left in the input buffer. */
    }
}

static bool read_line(const char *prompt, char *buffer, size_t size) {
    size_t length;

    if (prompt != NULL) {
        printf("%s", prompt);
    }

    if (fgets(buffer, (int) size, stdin) == NULL) {
        return false;
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        discard_remaining_input();
    }

    return true;
}

static bool read_menu_choice(int *choice) {
    char input[64];
    char *end;
    long value;

    if (!read_line("Choose an option: ", input, sizeof(input))) {
        return false;
    }

    errno = 0;
    value = strtol(input, &end, 10);

    while (isspace((unsigned char) *end)) {
        end++;
    }

    if (errno == ERANGE || end == input || *end != '\0' ||
        value < INT_MIN || value > INT_MAX) {
        *choice = -1;
    } else {
        *choice = (int) value;
    }

    return true;
}

/* ---------------------------- Polynomial ADT ----------------------------- */

static Term *create_term(long long coefficient, int exponent) {
    Term *term = xmalloc(sizeof(*term));

    term->coefficient = coefficient;
    term->exponent = exponent;
    term->next = NULL;

    return term;
}

static void free_polynomial(Term *polynomial) {
    while (polynomial != NULL) {
        Term *next = polynomial->next;
        free(polynomial);
        polynomial = next;
    }
}

static Term *clone_polynomial(const Term *polynomial) {
    Term *copy = NULL;
    Term **tail = &copy;

    while (polynomial != NULL) {
        *tail = create_term(polynomial->coefficient, polynomial->exponent);
        tail = &((*tail)->next);
        polynomial = polynomial->next;
    }

    return copy;
}

/*
 * Inserts a term in descending exponent order.
 * If the exponent already exists, the coefficients are combined.
 * A zero coefficient term is removed from the polynomial.
 */
static void insert_or_add_term(Term **polynomial,
                               long long coefficient,
                               int exponent) {
    Term **current;

    if (coefficient == 0) {
        return;
    }

    current = polynomial;

    while (*current != NULL && (*current)->exponent > exponent) {
        current = &((*current)->next);
    }

    if (*current != NULL && (*current)->exponent == exponent) {
        (*current)->coefficient += coefficient;

        if ((*current)->coefficient == 0) {
            Term *zero_term = *current;
            *current = zero_term->next;
            free(zero_term);
        }

        return;
    }

    {
        Term *new_term = create_term(coefficient, exponent);
        new_term->next = *current;
        *current = new_term;
    }
}

static void append_term(Term ***tail,
                        long long coefficient,
                        int exponent) {
    if (coefficient == 0) {
        return;
    }

    **tail = create_term(coefficient, exponent);
    *tail = &((**tail)->next);
}

static Term *add_or_subtract_polynomials(const Term *left,
                                         const Term *right,
                                         int right_sign) {
    Term *result = NULL;
    Term **tail = &result;

    while (left != NULL || right != NULL) {
        if (right == NULL ||
            (left != NULL && left->exponent > right->exponent)) {
            append_term(&tail, left->coefficient, left->exponent);
            left = left->next;
        } else if (left == NULL || right->exponent > left->exponent) {
            append_term(&tail,
                        right_sign * right->coefficient,
                        right->exponent);
            right = right->next;
        } else {
            long long coefficient =
                left->coefficient + right_sign * right->coefficient;

            append_term(&tail, coefficient, left->exponent);
            left = left->next;
            right = right->next;
        }
    }

    return result;
}

static Term *add_polynomials(const Term *left, const Term *right) {
    return add_or_subtract_polynomials(left, right, 1);
}

static Term *subtract_polynomials(const Term *left, const Term *right) {
    return add_or_subtract_polynomials(left, right, -1);
}

static Term *multiply_polynomials(const Term *left, const Term *right) {
    Term *result = NULL;
    const Term *first;

    for (first = left; first != NULL; first = first->next) {
        const Term *second;

        for (second = right; second != NULL; second = second->next) {
            insert_or_add_term(&result,
                               first->coefficient * second->coefficient,
                               first->exponent + second->exponent);
        }
    }

    return result;
}

static void print_polynomial(FILE *stream, const Term *polynomial) {
    bool first_term = true;

    if (polynomial == NULL) {
        fprintf(stream, "0");
        return;
    }

    while (polynomial != NULL) {
        long long coefficient = polynomial->coefficient;
        unsigned long long absolute_coefficient;

        if (first_term) {
            if (coefficient < 0) {
                fprintf(stream, "-");
            }
        } else {
            fprintf(stream, coefficient < 0 ? " - " : " + ");
        }

        absolute_coefficient = coefficient < 0
            ? (unsigned long long) (-(coefficient + 1)) + 1ULL
            : (unsigned long long) coefficient;

        if (polynomial->exponent == 0) {
            fprintf(stream, "%llu", absolute_coefficient);
        } else {
            if (absolute_coefficient != 1ULL) {
                fprintf(stream, "%llu", absolute_coefficient);
            }

            fprintf(stream, "x");

            if (polynomial->exponent != 1) {
                fprintf(stream, "^%d", polynomial->exponent);
            }
        }

        first_term = false;
        polynomial = polynomial->next;
    }
}

/* --------------------------- Polynomial parser --------------------------- */

static void skip_spaces(const char **cursor) {
    while (isspace((unsigned char) **cursor)) {
        (*cursor)++;
    }
}

static Term *parse_polynomial(const char *text, bool *is_valid) {
    Term *polynomial = NULL;
    const char *cursor = text;

    *is_valid = true;

    while (true) {
        int sign = 1;
        long long coefficient = 1;
        int exponent;
        bool has_number = false;
        char *end;

        skip_spaces(&cursor);

        if (*cursor == '\0') {
            break;
        }

        if (*cursor == '+' || *cursor == '-') {
            sign = (*cursor == '-') ? -1 : 1;
            cursor++;
            skip_spaces(&cursor);
        }

        if (*cursor == '\0') {
            *is_valid = false;
            break;
        }

        if (isdigit((unsigned char) *cursor)) {
            errno = 0;
            coefficient = strtoll(cursor, &end, 10);

            if (errno == ERANGE) {
                *is_valid = false;
                break;
            }

            cursor = end;
            has_number = true;
            skip_spaces(&cursor);
        }

        if (*cursor == 'x' || *cursor == 'X') {
            exponent = 1;
            cursor++;
            skip_spaces(&cursor);

            if (*cursor == '^') {
                long parsed_exponent;

                cursor++;
                skip_spaces(&cursor);

                if (!isdigit((unsigned char) *cursor)) {
                    *is_valid = false;
                    break;
                }

                errno = 0;
                parsed_exponent = strtol(cursor, &end, 10);

                if (errno == ERANGE || parsed_exponent > INT_MAX) {
                    *is_valid = false;
                    break;
                }

                exponent = (int) parsed_exponent;
                cursor = end;
            }
        } else {
            if (!has_number) {
                *is_valid = false;
                break;
            }

            exponent = 0;
        }

        insert_or_add_term(&polynomial,
                           sign * coefficient,
                           exponent);

        skip_spaces(&cursor);

        if (*cursor != '\0' && *cursor != '+' && *cursor != '-') {
            *is_valid = false;
            break;
        }
    }

    if (!*is_valid) {
        free_polynomial(polynomial);
        return NULL;
    }

    return polynomial;
}

/* ------------------------------- Stack ADT ------------------------------- */

static void stack_push(StackNode **top, Term *polynomial) {
    StackNode *new_node = xmalloc(sizeof(*new_node));

    new_node->polynomial = polynomial;
    new_node->next = *top;
    *top = new_node;
}

static Term *stack_pop(StackNode **top) {
    StackNode *node;
    Term *polynomial;

    if (*top == NULL) {
        return NULL;
    }

    node = *top;
    polynomial = node->polynomial;
    *top = node->next;
    free(node);

    return polynomial;
}

static void free_stack(StackNode *top) {
    while (top != NULL) {
        StackNode *next = top->next;
        free_polynomial(top->polynomial);
        free(top);
        top = next;
    }
}

static StackNode *clone_stack(const StackNode *top) {
    StackNode *copy;

    if (top == NULL) {
        return NULL;
    }

    copy = xmalloc(sizeof(*copy));
    copy->polynomial = clone_polynomial(top->polynomial);
    copy->next = clone_stack(top->next);

    return copy;
}

static void print_stack_in_input_order(const StackNode *node, int *number) {
    if (node == NULL) {
        return;
    }

    print_stack_in_input_order(node->next, number);
    printf("P%d(x) = ", *number);
    print_polynomial(stdout, node->polynomial);
    printf("\n");
    (*number)++;
}

/* -------------------------- File and operations -------------------------- */

static bool load_polynomials_from_file(const char *file_name,
                                       StackNode **loaded_stack,
                                       size_t *loaded_count) {
    FILE *file = fopen(file_name, "r");
    StackNode *temporary_stack = NULL;
    char line[MAX_LINE_LENGTH];
    size_t line_number = 0;
    size_t count = 0;

    if (file == NULL) {
        perror("Could not open input file");
        return false;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        bool is_valid;
        Term *polynomial;
        size_t length;

        line_number++;
        length = strlen(line);

        if (length > 0 && line[length - 1] != '\n' && !feof(file)) {
            fprintf(stderr,
                    "Error: line %zu is longer than %d characters.\n",
                    line_number,
                    MAX_LINE_LENGTH - 1);
            free_stack(temporary_stack);
            fclose(file);
            return false;
        }

        line[strcspn(line, "\r\n")] = '\0';

        {
            const char *cursor = line;
            skip_spaces(&cursor);

            if (*cursor == '\0') {
                continue;
            }
        }

        polynomial = parse_polynomial(line, &is_valid);

        if (!is_valid) {
            fprintf(stderr,
                    "Error: invalid polynomial on line %zu: %s\n",
                    line_number,
                    line);
            free_stack(temporary_stack);
            fclose(file);
            return false;
        }

        stack_push(&temporary_stack, polynomial);
        count++;
    }

    if (ferror(file)) {
        perror("Error while reading input file");
        free_stack(temporary_stack);
        fclose(file);
        return false;
    }

    fclose(file);

    if (count == 0) {
        fprintf(stderr, "Error: the input file contains no polynomials.\n");
        free_stack(temporary_stack);
        return false;
    }

    *loaded_stack = temporary_stack;
    *loaded_count = count;
    return true;
}

static Term *apply_operation(const Term *left,
                             const Term *right,
                             Operation operation) {
    switch (operation) {
        case OP_ADD:
            return add_polynomials(left, right);
        case OP_SUBTRACT:
            return subtract_polynomials(left, right);
        case OP_MULTIPLY:
            return multiply_polynomials(left, right);
        default:
            return NULL;
    }
}

/*
 * Executes the operation in stack pairs until one polynomial remains.
 * For p, q, r loaded in that order:
 *   first  = q operation r
 *   result = p operation first
 */
static Term *reduce_all_polynomials(const StackNode *source,
                                    Operation operation) {
    StackNode *work = clone_stack(source);

    while (work != NULL && work->next != NULL) {
        Term *right = stack_pop(&work);
        Term *left = stack_pop(&work);
        Term *combined = apply_operation(left, right, operation);

        free_polynomial(left);
        free_polynomial(right);
        stack_push(&work, combined);
    }

    return stack_pop(&work);
}

static const char *operation_name(Operation operation) {
    switch (operation) {
        case OP_ADD:
            return "Addition";
        case OP_SUBTRACT:
            return "Subtraction";
        case OP_MULTIPLY:
            return "Multiplication";
        default:
            return "Unknown operation";
    }
}

static void print_menu(void) {
    printf("\n========== Polynomial Stack Menu ==========\n");
    printf("1. Load the polynomials file\n");
    printf("2. Print the polynomials\n");
    printf("3. Add all polynomials\n");
    printf("4. Subtract all polynomials\n");
    printf("5. Multiply all polynomials\n");
    printf("6. Print the latest result to the screen\n");
    printf("7. Print the latest result to a file\n");
    printf("8. Exit\n");
    printf("===========================================\n");
}

int main(void) {
    StackNode *polynomial_stack = NULL;
    Term *latest_result = NULL;
    size_t polynomial_count = 0;
    bool result_available = false;
    Operation latest_operation = OP_ADD;
    int choice = 0;

    while (choice != 8) {
        print_menu();

        if (!read_menu_choice(&choice)) {
            printf("\nEnd of input. Exiting program.\n");
            break;
        }

        switch (choice) {
            case 1: {
                char file_name[MAX_FILE_NAME];
                StackNode *new_stack = NULL;
                size_t new_count = 0;

                if (!read_line("Enter the input file name: ",
                               file_name,
                               sizeof(file_name))) {
                    printf("Could not read the file name.\n");
                    break;
                }

                if (load_polynomials_from_file(file_name,
                                               &new_stack,
                                               &new_count)) {
                    free_stack(polynomial_stack);
                    polynomial_stack = new_stack;
                    polynomial_count = new_count;

                    free_polynomial(latest_result);
                    latest_result = NULL;
                    result_available = false;

                    printf("Successfully loaded %zu polynomial%s.\n",
                           polynomial_count,
                           polynomial_count == 1 ? "" : "s");
                }
                break;
            }

            case 2: {
                int number = 1;

                if (polynomial_stack == NULL) {
                    printf("No polynomials are loaded.\n");
                    break;
                }

                printf("\nLoaded polynomials:\n");
                print_stack_in_input_order(polynomial_stack, &number);
                break;
            }

            case 3:
            case 4:
            case 5: {
                Operation operation = choice == 3
                    ? OP_ADD
                    : (choice == 4 ? OP_SUBTRACT : OP_MULTIPLY);

                if (polynomial_stack == NULL) {
                    printf("Load a polynomial file first.\n");
                    break;
                }

                free_polynomial(latest_result);
                latest_result = reduce_all_polynomials(polynomial_stack,
                                                       operation);
                latest_operation = operation;
                result_available = true;

                printf("%s completed for all %zu polynomial%s.\n",
                       operation_name(operation),
                       polynomial_count,
                       polynomial_count == 1 ? "" : "s");
                break;
            }

            case 6:
                if (!result_available) {
                    printf("No result is available yet.\n");
                } else {
                    printf("%s result: ", operation_name(latest_operation));
                    print_polynomial(stdout, latest_result);
                    printf("\n");
                }
                break;

            case 7: {
                char file_name[MAX_FILE_NAME];
                FILE *output_file;

                if (!result_available) {
                    printf("No result is available yet.\n");
                    break;
                }

                if (!read_line("Enter the output file name: ",
                               file_name,
                               sizeof(file_name))) {
                    printf("Could not read the file name.\n");
                    break;
                }

                output_file = fopen(file_name, "w");

                if (output_file == NULL) {
                    perror("Could not open output file");
                    break;
                }

                print_polynomial(output_file, latest_result);
                fprintf(output_file, "\n");

                if (fclose(output_file) == EOF) {
                    perror("Could not close output file");
                } else {
                    printf("Result saved successfully to %s.\n", file_name);
                }
                break;
            }

            case 8:
                printf("Goodbye.\n");
                break;

            default:
                printf("Please enter a number from 1 to 8.\n");
                break;
        }
    }

    free_polynomial(latest_result);
    free_stack(polynomial_stack);

    return EXIT_SUCCESS;
}
