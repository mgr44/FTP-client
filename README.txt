Michael Ritchie
ftp client

Note, I am hosting this program as example of work. If you are a student at UW do not use
this code in your own asingments.

Complie using g++ *.cpp
ftp.cpp depends on socket.h and socket.cpp

Socket.h and socket.cpp were given to the winter 2017 netwroking class for use by the students in
out assignments. These files provide functionality to open client and server sockets and return
the file decriptors for the connections. Only the basic getClientSocket() function was used for the
FTP client.

Instructions on use:
The program can be run with an ip address passed in as an argument, in which case it will
atomatically open a connection to that address. After which the user will be prompted for a username
and password. 

If no arguments are given the user can open a conncection by typing open, followed by the ip the
user whishes to connect to.

Once connection is estalbished the user will be prompted for other commands such as c, list, put and get

Incorrect client commands will be handeled, however no error checking is performed on commands sent to
the FTP server and errors will produce undefined results.