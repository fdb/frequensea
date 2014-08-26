reader: reader.c
	gcc -I /opt/local/include -L /opt/local/lib -l hackrf -o reader reader.c

sender: sender.c
	gcc -I /opt/local/include -L /opt/local/lib -l hackrf -o sender sender.c

vis: vis.c
	gcc -I /opt/local/include -L /opt/local/lib -o vis vis.c  -l glfw -l hackrf -lpng -framework OpenGL


