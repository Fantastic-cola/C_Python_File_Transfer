# Instructions
To compile Server: gcc ftserver.c

Server should have at least one text file in its current directory.


# Usage
Server: ./a.out [SERVER PORT]
Client: python ftclient.py [HOSTNAME] [SERVER PORT] [COMMAND] [DATA PORT] <FILENAME>

Server takes a port number which listens for connections from Clients. Server must be run first. 

Client takes a hostname and a server port number, which indicates the Server that the Client wants to connect to.
Client takes a data port number, which indicates the port that the Client will receive the filename(s) or file from the Server.
Client also has a command argument, which should only be [-l] or [-g].
* [-l] request a list of all the files in the server's current directory
* [-g] request a file with the specified <FILENAME>


# Resources
C:
http://beej.us/guide/bgnet/

http://www.linuxhowtos.org/C_C++/socket.htm

CS344 - Lecture Notes & Project 4	
https://stackoverflow.com/questions/7483301/how-do-i-print-client-address-while-running-server-code-in-socket	

http://www.studytonight.com/c/program-to-list-files-in-directory.php

https://stackoverflow.com/questions/5190553/linux-c-get-server-hostname
	
Python:
Computer Networking: A Top-Down Approach 7th ed.
https://stackoverflow.com/questions/7113032/overloaded-functions-in-python

http://www.jython.org/docs/library/struct.html

https://stackoverflow.com/questions/82831/how-do-i-check-whether-a-file-exists-using-python
	
	
