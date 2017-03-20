#include "Socket.h"
#include <stdlib.h>
#include <sys/poll.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include "Timer.h"

using namespace std;

int openTCP( char ipName[], int portNumber); //returns the file decriptor number
void authenticate();
void setUserName();
void sendPassword();
void getSystemInfo();
void listFiles();
void changeDirectory();
void putFile();
void getFile();
void closeConnection();
int passiveOpen(); //returns the port number for the data connection
void readResponse(int fd, ostream &outStream);
char* getUserInput(string command);
char* convertString(string toConvert);

Socket *sock;
char* ipAddr;
int commandFD = -1;
int dataFD;
struct pollfd ufds;

int main( int argc, char* argv[] ) {

  if(argc > 1)//open a connection to argv[1]
  { 
    commandFD = openTCP(argv[1], 21); //open a tcp connection using Socket.cpp

    if(commandFD <0)//invalid address returns -1
    { cout << "invalid address" << endl;}
    else
    { 
      readResponse(commandFD, cout);
      authenticate();
    }
  }

  string input;
  while(true)//loop until the exit command is given
  {
    if(commandFD < 0)//connection hasn't been established
    {
      //print message instrucing to use open command or exit
      cout << "Enter: (open <ipAddress>) to open a connection" << endl
        << "Enter: (quit) to quit." << endl;
      //get an input
      cin >> input; 

      if(input == "open") //open command was givien
      {
        //open a connection
        string addr;
        cin >> addr; //get the ip address from the user
        char* ipAddress = convertString(addr);//convert that string to a usable char*      
        commandFD =openTCP(ipAddress, 21); //open a tcp connection using socket.cpp

        if(commandFD <0)//invalid address returns -1
        { cout << "invalid address" << endl;}
        else
        { 
          readResponse(commandFD, cout);
          authenticate(); //send username and password
        }
        //delete ipAddress; //delete pointer memory
      }
    }
    else
    {
      cout << "Valid commands are: (cd <sub_folder>), (ls), (get <file_name>), (put), (close), and (quit)" << endl;
      cin >> input;
      //handel other command values
      if(input == "cd")
        {changeDirectory();}
      else if (input == "ls")
        {listFiles();}
      else if (input == "get")
        {getFile();;}
      else if (input == "put")
        {putFile();}
      else if (input == "close")
        {closeConnection();}
      else if (input == "open")
        {cout <<"connection already open" << endl;}
      else
        {cout << "invalid input" << endl;}        
    }

    if(input == "quit")//exit command was given
    { 
      //check if connection is alread closed
      if(commandFD != -1)
      {
        closeConnection();
      }
      delete sock; // delete pointer memory
      //delete ipAddr;
      cout << "Finished" << endl;
      break;
    }
  }
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
//openTCP()
///////////////////////////////////////////////////////////////////////////////
int openTCP(char address[], int portNumber ) {

  sock = new Socket(portNumber); // connect to the port

  //saving global ip for later use.
  ipAddr = new char[strlen(address)];
  strcpy(ipAddr, address);

  // Get a client sd
  cout << "opening tcp connection to port: " << portNumber << endl;
  return sock->getClientSocket( address );
  delete sock;
  
}

///////////////////////////////////////////////////////////////////////////////
//authenticate()
///////////////////////////////////////////////////////////////////////////////
void authenticate()
{
    setUserName();
    sendPassword();
    getSystemInfo();
}


///////////////////////////////////////////////////////////////////////////////
//setUserName()
///////////////////////////////////////////////////////////////////////////////
void setUserName()
{
	cout << "Please enter username: ";
  char* fullCommand = getUserInput("USER");

  cout << "Send USER command through tcp connection" << endl;
  write(commandFD, fullCommand, strlen(fullCommand)); //send the USER command with the username
  readResponse(commandFD, cout);
  //delete fullCommand;
}


///////////////////////////////////////////////////////////////////////////////
//sendPassword()
///////////////////////////////////////////////////////////////////////////////
void sendPassword()
{
  cout << "Please enter password: ";
  char* fullCommand = getUserInput("PASS");
  cout << "Send PASS command through tcp connection" << endl;
  write(commandFD, fullCommand, strlen(fullCommand)); // send the PASS command with the pasword
  readResponse(commandFD, cout);
  //delete fullCommand;
}


///////////////////////////////////////////////////////////////////////////////
//getSystemInfo()
///////////////////////////////////////////////////////////////////////////////
void getSystemInfo()
{
  char command[] = "SYST\r\n";
  write(commandFD, command, strlen(command)); // send the SYST command
  readResponse(commandFD, cout);
}


///////////////////////////////////////////////////////////////////////////////
//listFiles()
///////////////////////////////////////////////////////////////////////////////
void listFiles()
{
  //send Pasv open command and get the port number
  int portNum = passiveOpen();

  //start a new tcp connection on the data connection port
  dataFD = openTCP(ipAddr, portNum);

  //fork the process
  if(fork() == 0)
  {
    //child should read from the data port, then close the connection, and exit
    readResponse(dataFD, cout);
    close(dataFD);
    exit(0);
  }
  else
  {
    //parent should send the list command then close it's end of the connection 
    cout << "Send LIST command through tcp connection" << endl;
    char command[] = "LIST\r\n";
    write(commandFD, command, strlen(command)); // send the LIST command

    readResponse(commandFD, cout);
    close(dataFD);
  }
}
void changeDirectory()
{
  char* fullCommand = getUserInput("CWD");

  cout << "Send CWD command through tcp connection" << endl;
  write(commandFD, fullCommand, strlen(fullCommand)); // send the CWD command
  readResponse(commandFD, cout);
  //delete fullCommand;
}


///////////////////////////////////////////////////////////////////////////////
//putFile() 
///////////////////////////////////////////////////////////////////////////////
void putFile()
{
  //open passive data connection port
  int portNumber = passiveOpen();

  //open tcp connection to specified port
  dataFD = openTCP(ipAddr, portNumber);

	//Send TYPE command over tcp connection
  char command[] = "TYPE I\r\n";
  write(commandFD, command, strlen(command));
  readResponse(commandFD, cout);

  //send STOR command over tcp connection
  cout << "Enter remote file name: ";
  char *fullCommand = getUserInput("STOR");  

  cout << "Send STOR command through tcp connection" << endl;
  write(commandFD, fullCommand, strlen(fullCommand)); // send the STOR command
  readResponse(commandFD, cout);
  //delete fullCommand;

  //get file contents
  char buffer[4096];
  cout << "Enter local file name: ";
  string temp;
  cin >> temp;
  char * localFileName = convertString(temp);
  int file = open( localFileName, O_RDONLY ); //open file
  //delete localFileName;

  cout << "send file data through data connection" << endl;

  int nread;
  while (true) //loop until there is no more data to read from the file
  {
    bzero(buffer, 4096);
    nread = read(file, buffer, 4096); //read some data from the file
    cout << "Read: " << nread << " bytes from file." << endl;
    if(nread > 0)
    {
      int nWritten = 0;
      while(nWritten < nread)//loop until all data has been written
      {
        nWritten += write(dataFD, buffer, nread); //write the data to the file
        cout << "Wrote: " << nWritten << " bytes to connection" << endl;
      }
    }
    else {break;} //no more data to read so break
  }
  //close connections
  close(file);
  close(dataFD);

  readResponse(commandFD, cout); //read response
}


///////////////////////////////////////////////////////////////////////////////
//getFile() 
///////////////////////////////////////////////////////////////////////////////
void getFile()
{
  //open passive data connection port
  int portNumber = passiveOpen();

  //open tcp connection to specified port
  dataFD = openTCP(ipAddr, portNumber);

  //get file name
  string fileStringName;
  cin >> fileStringName;
  char *localFileName = convertString(fileStringName); //convert filename into usable format

  //setup variables for read and open file
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  cout <<"file name is: " << localFileName << endl;
  int file = open( localFileName, O_WRONLY | O_CREAT, mode );

  char buffer[4096];                //maximum size read
  ufds.fd = dataFD;                 // a socket descriptor to exmaine for read
  ufds.events = POLLIN;             // check if this sd is ready to read
  ufds.revents = 0;                 // simply zero-initialized
  bzero(buffer, 4096);

  //Timer* myTimer = new Timer();
  //myTimer->start();
  //fork a new process
  if(fork() == 0)
  {
    //child process should read data coming in
    //do one blocking read to get the first bit of data
    cout << "waiting for file data..." << endl;
    int nread = read( dataFD, buffer, 4096 );
    write(file, buffer, nread); //write data to the file
    while(true)
   {
      ufds.revents = 0;                 // simply zero-initialized
      bzero(buffer, 4096);

      int val = poll( &ufds, 1, 1000 ); // poll this socket for 1000msec (=1sec)

      if ( val > 0 ) {
         nread = read( dataFD, buffer, 4096 ); // read from connection
         write(file, buffer, nread); //write data to the file
      }
		  else {break;}
  }
    
    //close the file descriptor and exit
    close(file);
    close(dataFD);
    exit(0);

  }
  else //parent process
  {
    //sleep for a little bit to make sure the cild process will be waiting to read the data
    sleep(1);//1 = one second

    //parent process should send command and read response
    char fullCommand[] = "RETR ";
    strcat(fullCommand, localFileName);
    char delim[] = "\r\n";
    strcat(fullCommand, delim);

    cout << "Send RETR command through tcp connection" << endl;
    write(commandFD, fullCommand, strlen(fullCommand)); // send the RETR command  
    readResponse(commandFD, cout);  
    //long time = myTimer->lap(myTimer->getSec(),myTimer->getUsec() );
    //cout << "Time: "  << time << endl;
    //delete myTimer;
    //close parents's data file descriptor
    close(dataFD);
    
  }
}


///////////////////////////////////////////////////////////////////////////////
//closeConnection()
///////////////////////////////////////////////////////////////////////////////
void closeConnection()
{
  if(commandFD < 0)
  {
    cout << "no open connection to close" << endl;
    return;
  }  

  cout << "Send QUIT command through tcp connection" << endl;
  char command[] = "QUIT\r\n";
  write(commandFD, command, strlen(command)); // send the QUIT command
  
  //read response depends on the connection, so do a simple read here
  char response[100];
  bzero(response, 100);
  read(commandFD, response, 100);
  cout << response << endl; //print response
  close(commandFD); //close file descriptor for connenction
  commandFD = -1;
}


///////////////////////////////////////////////////////////////////////////////
//passiveOpen()
//returns and int of the port number the server will be listening on
///////////////////////////////////////////////////////////////////////////////
int passiveOpen() //send PASV command to server and return the port number
{
  //send pasv command to the server
  char command[] = "PASV\r\n";
  cout << "Send PASV command through the tcp connection" << endl;
  write(commandFD, command, strlen(command)); // send the SYST command

  //read the server's response and print it
  char response[100];
  bzero(response, 100);
  read(commandFD, response, 100);
  cout << response << endl;

  //calculate the data port to connect too
  char fithNum[4];
  char sixthNum[4];
  bzero(fithNum, 4);
  bzero(sixthNum, 4);
  int offset = 0;
  int numCommas = 0;
  for(int i = 0; i < 100; i++) //loop through the response to get to the embeded port number
  { 
    if(response[i] == ',')
    {
      numCommas++;
      offset = 0;
    }
    else if (response[i] == ')')// reached the end of the message so break
    {
      break; 
    }
    else if (numCommas == 4) // read the data after 4th comma for part one of the port number
    {
      fithNum[offset] = response[i];
      offset++;
    }
    else if(numCommas == 5) //read the data after the 5th comma for part two of the port number
    {
      sixthNum[offset] = response[i];
      offset++;
    }
  } 
  //cout << "5th: " << fithNum << endl << "6th: " << sixthNum << endl;
  int partOne = atoi(fithNum);
  int partTwo = atoi(sixthNum);
  int portNumber = (partOne * 256) + partTwo;
  cout << "Port Number: " << portNumber << endl;
  return portNumber;
}


///////////////////////////////////////////////////////////////////////////////
//readResponse()
//fd is the file descriptor the function will read from
// outStream is the stream the function will ouput to
///////////////////////////////////////////////////////////////////////////////
void readResponse(int fd, ostream &outStream)
{
  char oneLineResponse[4096];        //maximum length line of text
  ufds.fd = fd;                     // a socket descriptor to exmaine for read
  ufds.events = POLLIN;             // check if this sd is ready to read

  //do one blocking read to get the first bit of data
  ufds.revents = 0;                 // simply zero-initialized
  bzero(oneLineResponse, 4096);
  cout << "waiting for response..." <<endl;
  int nread = read( fd, oneLineResponse, 4096 );
  outStream << oneLineResponse;  

  while(true)
  {
    ufds.revents = 0;                 // simply zero-initialized
    bzero(oneLineResponse, 4096);

    int val = poll( &ufds, 1, 1000 ); // poll this socket for 1000msec (=1sec)
    if ( val > 0 ) 
    { 
      int nread = read( fd, oneLineResponse, 4096 ); //read from connectio 
      outStream << oneLineResponse; //print to out stream
    }
		else
		{
			cout << endl;
			break;
		}
  }
}


///////////////////////////////////////////////////////////////////////////////
//getUserInput()
///////////////////////////////////////////////////////////////////////////////
char* getUserInput(string command)
{
  string endingSequence = "\r\n";
  string input;
  cin >> input;

  //assemple a full command and return it in the form of a char pointer
  command += " ";
  command += input;
  command += endingSequence;
  return convertString(command);
}
///////////////////////////////////////////////////////////////////////////////
//convertString()
//takes a string and converts it to a char* representing an array
///////////////////////////////////////////////////////////////////////////////
char* convertString(string toConvert)
{
  char* arrayPointer = new char[toConvert.length()];
  strcpy(arrayPointer, toConvert.c_str());
  return arrayPointer;
}
