# Polynomial ADT Using Linked List and Stack

A C application that represents and processes polynomials using linked lists and a stack data structure.

This project was originally developed as part of the Data Structures course and was later reviewed and improved for documentation and portfolio purposes.

## Features

- Load multiple polynomials from a text file
- Represent each polynomial using a linked list
- Store polynomials inside a stack
- Add all loaded polynomials
- Subtract all loaded polynomials
- Multiply all loaded polynomials
- Display the original polynomials
- Display the latest operation result
- Save the result to an output file
- Validate user input and handle file errors

## Data Structures Used

### Linked List

Each polynomial is represented as a linked list. Every node stores:

- Coefficient
- Exponent
- Pointer to the next term

### Stack

The loaded polynomials are stored inside a stack implemented using a linked list.

## Project Files

```text
.
├── main.c
├── in.txt
├── README.md
└── .gitignore