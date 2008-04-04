#include "Connection.h"

/* constructors */
Connection::Connection(bool remote) {
	this->remote = remote;

	/* set up pipes */
	if (pipe(link) < 0) {
		cerr << "Plumbing failed" << endl;
		exit(1);
	}

	if (remote)
		usleep_time = REMOTE_USLEEP;
	else
		usleep_time = LOCAL_USLEEP;
	/* set up pty */
	int fd = 0;
	char slave[256];
	memset(slave, '\0', 256);
	winsize wsize;
	wsize.ws_row = ROWS;
	wsize.ws_col = COLS;
	wsize.ws_xpixel = 640;
	wsize.ws_ypixel = 480;
	pid_t pid = forkpty(&fd, slave, NULL, &wsize);
	if (pid == -1) {
		cerr << "There is no fork" << endl;
		exit(2);
	} else if (pid) {
		/* main thread */
		/* fix plumbing */
		link[0] = fd; // reading
		link[1] = fd; // writing
	} else {
		/* this is our pty, start nethack here */
		int result;
		if (remote)
			result = execl(REMOTE_NETHACK, REMOTE_NETHACK, REMOTE_NETHACK_URL, NULL);
		else
			result = execl(LOCAL_NETHACK, LOCAL_NETHACK, NULL);
		if (result < 0) {
			cerr << "Unable to enter the dungeon" << endl;
			exit(3);
		}
		return;
	}
	/* login remote */
	if (remote) {
		ifstream account;
		account.open(".account");
		if (!account) {
			cerr << "Unable to read .account (which should contain username and password on separate lines)" << endl;
			exit(99);
		}
		char username[32];
		char password[32];
		account >> username;
		account >> password;
		account.close();
		sleep(2);
		char buffer[BUFFER_SIZE];
		ssize_t size = retrieve(buffer, BUFFER_SIZE);
		cerr << buffer << endl;
		send("l");
		size = retrieve(buffer, BUFFER_SIZE);
		cerr << buffer << endl;
		send(username);
		send("\n");
		size = retrieve(buffer, BUFFER_SIZE);
		cerr << buffer << endl;
		send(password);
		send("\n");
		size = retrieve(buffer, BUFFER_SIZE);
		cerr << buffer << endl;
		send("p");
		sleep(3);
		cerr << "Done logging in, a game should appear for the bot now" << endl;
	}
}

/* methods */
ssize_t Connection::retrieve(char *buffer, size_t count) {
	/* retrieve data */
	ssize_t data_received = 0;
	ssize_t data_received_total = 0;
	/* make reading blocking */
	fcntl(link[0], F_SETFL, fcntl(link[0], F_GETFL) & ~O_NONBLOCK);
	/* read 1 byte, this will block until there's data available */
	++data_received_total;
	data_received = read(link[0], buffer, data_received_total);
	/* make reading non-blocking */
	fcntl(link[0], F_SETFL, fcntl(link[0], F_GETFL) | O_NONBLOCK);
	data_received = read(link[0], &buffer[data_received_total], count - data_received_total);
	if (data_received != -1)
		data_received_total += data_received;
	cerr << "reading " << data_received_total << " | " << data_received << endl;
	for (int a = 0; a < data_received_total; ++a)
		cerr << buffer[a];
	cerr << endl;
	if (data_received_total < (ssize_t) count)
		buffer[data_received_total] = '\0';
	return data_received_total;
}

ssize_t Connection::send(const char *buffer) {
	/* send data */
	return write(link[1], buffer, strlen(buffer));
}