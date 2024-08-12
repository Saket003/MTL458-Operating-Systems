### Done
- First full run of assignment pdf
- Checked Piazza 10-08 1:30 AM

### Notes
For all types of error
OUTPUT: "Invalid Command"
without "

Yes you can assume directories and file names won’t have spaces in them

Output redirection won't be tested

> echo $HOME
Expected Output: $HOME

It should work like the actual history command. You can see the output for history in your shell itself. The actual history command lists the past commands as a numbered list, for this assignment you must not print the numbers along the commands but just the commands.

Yes, it should be able to handle the whitespaces between the flags and commands also. For e.g. your shell should be able to run commands like "ls.   -1", "wc     -l  file.txt" and "echo       hello".

You can assume that there are no wildcards in test case commands.

cd . .. ...

You can assume that cd command won't be a part of any pipe expression.

If the command is echo xyz you need to output xyz, assume there is at most one contiguous space in the input xyz.
history also with invalid commands

The range of #commands in test cases is such that the memory does not go out of range . Although there should not be any maximum length of command specified by the user . You can assume max_length to be 2048 characters.

In one run of shell, maximum how many commands should our code accept?
Until the user puts "exit" command.The range of #commands in test cases is such that the memory does not go out of range.