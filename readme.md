start  -> list eof
list   -> stmt list | ε

stmt   -> expr ; 
       | comment

comment -> # .* \n     { skip() }

expr   -> expr + term        { print('+') }
       | expr - term         { print('-') }
       | term

term   -> term * factor      { print('*') }
       | term / factor       { print('/') }
       | term div factor     { print('DIV') }
       | term mod factor     { print('MOD') }
       | term % factor       { print('%') }
       | factor

factor -> ( expr )
       | id                 { print(id.lexeme) }
       | num                { print(num.value) }



------------------


start   -> list eof
list    -> stmt list | ε

stmt    -> expr ; 
        | comment

comment -> # .* \n               { skip() }

expr    -> term expr'

expr'   -> + term                { print('+') } expr'
        | - term                { print('-') } expr'
        | ε

term    -> factor term'

term'   -> * factor             { print('*') } term'
        | / factor             { print('/') } term'
        | div factor           { print('DIV') } term'
        | mod factor           { print('MOD') } term'
        | % factor             { print('%') } term'
        | ε

factor  -> ( expr )
        | id                   { print(id.lexeme) }
        | num                  { print(num.value) }