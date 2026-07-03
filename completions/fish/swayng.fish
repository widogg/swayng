# swayng(1) completion

complete -f -c swayng
complete -c swayng -s h -l help --description "Show help message and quit."
complete -c swayng -s c -l config --description "Specifies a config file." -r
complete -c swayng -s C -l validate --description "Check the validity of the config file, then exit."
complete -c swayng -s d -l debug --description "Enables full logging, including debug information."
complete -c swayng -s v -l version --description "Show the version number and quit."
complete -c swayng -s V -l verbose --description "Enables more verbose logging."
complete -c swayng -l get-socketpath --description "Gets the IPC socket path and prints it, then exits."
