mysh: mysh.c
	gcc -g -Wall -fsanitize=address,undefined -o mysh mysh.c -I.  