Static File Server
---

This is a simple server which you can access the static files under ./environment directory

# Prerequisite
For all the examples, you should initialize the Plumber environment first. To enter the Plumber environment, type the following command under the root directory of **this repository (not this example directory)**

	./init

# How to make?
It's simple, first, you need to change pwd to the example directory and make 

	cd ${ENVROOT}/../src/fileserver && make

# How to start?
You can start the server by typing command

	./fileserver.pss

# How to try the server?
Please go to http://localhost:8080, you should see the welcome page. You should able to
put any other files to the ./environment/server_files directory, and access it via the 
server. 

You can find more details about how this example works in the page 
http://localhost:8080/explained_fileserver_pss.html

