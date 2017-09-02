#!/bin/python
"""
Author: 	Bolan Peng
Project 2: 	ftclient.py
Date:		8/6/2017
References:	Computer Networking: A Top-Down Approach 7th ed.
"""

from socket import *
from struct import *
import sys
import os.path

#############
# Functions
############# 

# Verify the validity of the port numbers
def verifyPortNum(port):
	if port < 1024 or port > 65535:
		return -1
	else:
		return port
		
# Opens a socket and establishes connection with the server
def establishConnection(hostname, serverPort):
	
	clientSocket = socket(AF_INET, SOCK_STREAM)
	try:
		clientSocket.connect((hostname, serverPort))
	except:
		print 'Cannot connect to ' + hostname + ':' + str(serverPort)
		sys.exit(1)
	return clientSocket


def receiveErrorMsg():
	print 'Error writing to socket.'
	controlSocket.close()
	sys.exit(1)


# REFERENCE: https://stackoverflow.com/questions/7113032/overloaded-functions-in-python
def sendCommand(controlSocket, dataPort, command, filename=None):	
	if filename is None:
		controlSocket.send(command)
		if controlSocket.recv(1024) != 'ok':
			receiveErrorMsg()
		
		controlSocket.send(str(dataPort))
		if controlSocket.recv(1024) != 'ok':
			receiveErrorMsg()
		
	else:
		controlSocket.send(command)
		if controlSocket.recv(1024) != 'ok':
			receiveErrorMsg()
			
		controlSocket.send(filename)
		if controlSocket.recv(1024) != 'ok':
			receiveErrorMsg()
		
		controlSocket.send(str(dataPort))
		if controlSocket.recv(1024) != 'ok':
			receiveErrorMsg()


#############
# Main
#############

if len(sys.argv) < 5 or len(sys.argv) > 6:
	print 'Usage: python ftclient.py [hostname] [server port] [command] [data port] <filename>'

else:
	# Store the arguments in named variables
	hostname = sys.argv[1]
	serverPort = int(sys.argv[2])
	command = sys.argv[3]
	dataPort = int(sys.argv[4])
	
	# Verify the validity of the port numbers
	if verifyPortNum(serverPort) == -1 or verifyPortNum(dataPort) == -1:
		print 'Invalid port number, please try again.'
		sys.exit(1)
	
	# Open a TCP control connection with the server port
	controlSocket = establishConnection(hostname, serverPort)
	
	# Start a TCP data connection on the data port
	dataSocket = socket(AF_INET, SOCK_STREAM)
	dataSocket.bind(('', dataPort))
	dataSocket.listen(1)
	
	# Only if command is '-g', get the filename, and send on control connection
	if command == '-g':	
		try:
			filename = sys.argv[5]
		except:
			print '[Filename] is required with command -g, please try again.'
			controlSocket.close()
			sys.exit(1)
		sendCommand(controlSocket, dataPort, command, filename)	
		
	# If command is anything else, directly send on control connection
	else:
		sendCommand(controlSocket, dataPort, command)
		
		
	# Receive response from server
	response = controlSocket.recv(1024)
	# if error
	if response == 'INVALID COMMAND':
		# Display error message, close controlSocket
		print response
		dataSocket.close()
		controlSocket.close()
		sys.exit(1)
	
	# if error in filename
	elif response == 'FILE NOT FOUND':
		# Display error message, close controlSocket
		print response
		dataSocket.close()
		controlSocket.close()
		sys.exit(1)
		
	# no error and command is -g	
	elif response == 'DATA':
		# receive data on data connection
		connectionSocket, addr = dataSocket.accept()
		print 'Receiving "' + filename + '" from ' + str(addr)
		
		# REFERENCE: http://www.jython.org/docs/library/struct.html
		dataSize = connectionSocket.recv(4)
		size = unpack('I', dataSize)
		data = connectionSocket.recv(size[0])
		
		# Check current directory for duplicate file name. If duplicate, add a number to the filename
		# REFERENCE: https://stackoverflow.com/questions/82831/how-do-i-check-whether-a-file-exists-using-python
		if os.path.isfile(filename):
			# Split the filename into name and extension
			name, extension = os.path.splitext(filename)
			name = name + '1'
			filename = name + extension
		
		# Otherwise, directly save the file
		file1 = open(filename, 'w')
		file1.write(data)
		file1.close()
		
		print 'File transfer complete.'
		
	# no error and command is -l
	else:
		# receive data on data connection
		connectionSocket, addr = dataSocket.accept()
		print 'Receiving directory structure from ', addr
		
		data = connectionSocket.recv(1024)
		print data 
		
		
	connectionSocket.close()
	dataSocket.close()
	controlSocket.close()
	sys.exit(0)
