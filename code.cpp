# include <stdio.h>
# include <ctype.h>
# include <stdlib.h>
# include <string.h>

// Some constants for buffer sizes and token identifiers
# define BSIZE 128
# define NONE -1
# define EOS '\0'
# define NUM 256
# define DIV 257
# define MOD 258
# define ID 259
# define DONE 260

# define SYMMAX 100
# define STRMAX 999

// Global variables used for lexical analysis and parsing
int tokenval = NONE;        // Holds the value of the current token
int lineno = 1;             // Line number in the input source
int lookahead;              // Lookahead token (for predictive parsing)

char lexbuf[BSIZE];         // Buffer to store the current lexeme
char lexemes[STRMAX];       // Array for storing all lexemes encountered
int lastchar = -1;          // Keeps track of the last character index in lexemes array
int lastentry = 0;          // Points to the last entry in the symbol table

FILE *infile;               // Input file
FILE *outfile;              // Output file

// Structure representing an entry in the symbol table
struct entry {
    const char *lexptr;  // Pointer to the string representing the lexeme
    int token;           // Token type for this entry
};

// Symbol table stores identifiers and keywords
struct entry symtable[SYMMAX];

// Function prototypes for lexical analysis and parsing
int lookup(const char *s); 
int insert(const char *s, int tok);
void error(const char *m);
int lexan();
void match(int t);
void expr();
void term();
void factor();
void emit(int t, int tval);
void parse();
void init();

// Lexical analyzer function: Reads the input and returns the next token
int lexan() {
    int t;
    
    while (1) {
        t = fgetc(infile);  // Get the next character from the input

        // Ignore whitespaces and tabs
        if (t == ' ' || t == '\t') 
            continue;
        
        // New line, increase the line number
        else if (t == '\n') 
            lineno++;

        // Comment: Skip everything until the end of the line
        else if (t == '#') {
            while ((t = fgetc(infile)) != '\n' && t != EOF);
            if (t == '\n') lineno++;
            continue;
        }
        
        // If the character is a digit, it's a number token
        else if (isdigit(t)) {
            ungetc(t, infile); // Push the character back to the input
            fscanf(infile, "%d", &tokenval); // Read the number
            return NUM;
        }

        // If it's an alphabetic character, it could be an identifier or keyword
        else if (isalpha(t)) {
            int p, b = 0;

            // Build the lexeme (identifier or keyword)
            while (isalnum(t)) {
                lexbuf[b++] = t;
                t = fgetc(infile);
                if (b >= BSIZE) error("Buffer overflow in lexeme"); // Prevent buffer overflow
            }
            
            lexbuf[b] = EOS;  // Null terminate the string

            // If the next character is not EOF, put it back in the input stream
            if (t != EOF) 
                ungetc(t, infile);

            // Lookup the identifier in the symbol table, or insert it if it's new
            p = lookup(lexbuf);
            if (p == 0)
                p = insert(lexbuf, ID);  // Insert as an identifier if not found

            tokenval = p;  // Store the index of the symbol in the tokenval
            return symtable[p].token; // Return the token type for the lexeme
        }

        // Handle special characters (modulo and division)
        else if (t == '%') {
            return MOD;
        }
        else if (t == '\\') {
            return '/';
        }

        // End of file, return DONE to signal parsing is finished
        else if (t == EOF) {
            return DONE;
        }
        
        // Return the character itself if it's a special symbol
        else {
            tokenval = NONE;
            return t;
        }
    }
}

// Parsing function: Starts the analysis and processes each statement
void parse() {
    lookahead = lexan();  // Get the first token
    while (lookahead != DONE) {  // Keep parsing until the end of input
        expr();  // Parse an expression
        match(';');  // Ensure the statement ends with a semicolon
        fprintf(outfile, ";\n");  // Output the semicolon to the file
    }
}

// Parse an expression: Handles addition and subtraction
void expr() {
    int t;
    term();  // Parse the first term (multiplication/division precedence)

    while (1) {
        switch (lookahead) {
            case '+': case '-':
                t = lookahead;  // Store the operator (+ or -)
                match(lookahead); term(); emit(t, NONE);  // Parse the next term and emit the operator
                continue;
            default:
                return;
        }
    }
}

// Parse a term: Handles multiplication, division, and modulo
void term() {
    int t;
    factor();  // Parse the first factor (numbers, identifiers, or parentheses)

    while (1) {
        switch (lookahead) {
            case '*': case '/': case DIV: case MOD:
                t = lookahead;  // Store the operator (*, /, DIV, MOD)
                match(lookahead); factor(); emit(t, NONE);  // Parse the next factor and emit the operator
                continue;
            default:
                return;
        }
    }
}

// Parse a factor: Handles parentheses, numbers, and identifiers
void factor() {
    switch (lookahead) {
        case '(':
            match('('); expr(); match(')'); break;  // Handle parentheses for grouping expressions
        case NUM:
            emit(NUM, tokenval); match(NUM); break;  // Handle number literals
        case ID:
            emit(ID, tokenval); match(ID); break;  // Handle identifiers (variable names)
        default:
            error("Syntax error: unexpected token in factor");
    }
}

// Match the current token with the expected token type
void match(int t) {
    if (lookahead == t) {
        lookahead = lexan();  // If matched, consume the token and look ahead
    } else {
        error("Syntax error: expected different token");
    }
}

// Emit the current token to the output file
void emit(int t, int tval) {
    switch (t) {
        case '+': case '-': case '*': case '/':
            fprintf(outfile, "%c ", t); break;  // For operators, just print the symbol
        case DIV:
            fprintf(outfile, "DIV "); break;  // Special case for DIV keyword
        case MOD:
            fprintf(outfile, "%% "); break;  // Special case for MOD keyword
        case NUM:
            fprintf(outfile, "%d ", tval); break;  // For numbers, print the value
        case ID:
            fprintf(outfile, "%s ", symtable[tval].lexptr); break;  // For identifiers, print the name
        default:
            fprintf(outfile, "token %d, tokenval %d ", t, tval);  // For unknown tokens, print debug info
    }
}

// Look up a lexeme in the symbol table and return its entry index, or 0 if not found
int lookup(const char *s) {
    int p;
    for (p = lastentry; p > 0; p--)  // Search symbol table for the lexeme
        if (strcmp(symtable[p].lexptr, s) == 0)
            return p;
    return 0;  // Not found
}

// Insert a new symbol into the symbol table
int insert(const char *s, int tok) {
    int len = strlen(s);  // Get the length of the lexeme
    if (lastentry + 1 >= SYMMAX)  // Check if symbol table is full
        error("Symbol table full");
    if (lastchar + len + 1 >= STRMAX)  // Check if lexemes array is full
        error("Lexemes array full");

    lastentry++;  // Increment last entry index
    symtable[lastentry].token = tok;  // Store the token type
    symtable[lastentry].lexptr = &lexemes[lastchar + 1];  // Point to the lexeme in the lexemes array
    lastchar += len + 1;  // Update the character position in the lexemes array
    strcpy((char *)symtable[lastentry].lexptr, s);  // Copy the lexeme to the lexemes array
    return lastentry;  // Return the new entry index
}

// Predefined keywords for the language
struct entry keywords[] = {
    {"div", DIV},  // Keyword for division
    {"mod", MOD},  // Keyword for modulo
    {0, 0}         // Sentinel value marking the end of the keyword list
};

// Initialize the symbol table with the predefined keywords
void init() {
    struct entry *p;
    for (p = keywords; p->token; p++)  // Insert each keyword into the symbol table
        insert(p->lexptr, p->token);
}

// Handle errors: Print error message and exit the program
void error(const char *m) {
    fprintf(stderr, "line %d: %s\n", lineno, m);  // Print the error with line number
    exit(1);  // Exit the program with failure
}

// Main function: Entry point of the compiler
int main(int argc, char *argv[]) {
    if (argc != 3) {  // Check if the correct number of arguments were provided
        fprintf(stderr, "Usage: sc infile.inf outfile.pos\n");
        exit(1);
    }

    // Open the input file for reading
    infile = fopen(argv[1], "r");
    if (!infile) {
        perror("Error opening input file");  // Print error if file can't be opened
        exit(1);
    }

    // Open the output file for writing
    outfile = fopen(argv[2], "w");
    if (!outfile) {
        perror("Error opening output file");  // Print error if file can't be opened
        fclose(infile);  // Don't forget to close the input file
        exit(1);
    }

    init();  // Initialize the symbol table with keywords
    parse();  // Start the parsing process

    // Close the input and output files
    fclose(infile);
    fclose(outfile);

    return 0;
}