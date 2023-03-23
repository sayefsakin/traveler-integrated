# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -Wall

# The build target 
TARGET = Server
CGAL_SERVER = cgal_data_server
INCLUDE = rapidjson/include
CLIENT = Client
CGET = curl_get

all: $(CGAL_SERVER).cpp
	$(RM) $(CGAL_SERVER)
	$(CC) $(CFLAGS) -o $(CGAL_SERVER) $(CGAL_SERVER).cpp $(CGET).cpp -I $(INCLUDE) -lcurl
	./$(CGAL_SERVER)

#cget: $(CGET)
#
#$(TARGET): $(TARGET).cpp
#	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp -I $(INCLUDE)
#
#$(CLIENT): $(CLIENT).cpp
#	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -I $(INCLUDE)
#
#$(CGET): $(CGET).cpp
#	$(CC) $(CFLAGS) -o $(CGET) $(CGET).cpp -I $(INCLUDE) -lcurl
#
#$(CGAL_SERVER): $(CGAL_SERVER).cpp
#	$(CC) $(CFLAGS) -o $(CGAL_SERVER) $(CGAL_SERVER).cpp -I $(INCLUDE) -lcurl

clean:
	$(RM) $(TARGET) $(CLIENT) $(CGET)