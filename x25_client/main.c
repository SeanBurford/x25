#include <linux/x25.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define X25_ADDR_LEN 16

void describe_x25(int sock) {
	struct x25_facilities facilities;
	memset(&facilities, 0, sizeof(facilities));
	if (ioctl(sock,SIOCX25GFACILITIES, &facilities) < 0) {
		perror("ioctl(SIOCX25GFACILITIES):");
		return;
	}
	printf("Packet sizes: in: %d, out: %d\n", 1<<facilities.pacsize_in, 1<<facilities.pacsize_out);
	printf("Window sizes: in: %d, out: %d\n", facilities.winsize_in, facilities.winsize_out);
}

void show_cause_x25(int sock) {
	struct x25_causediag causediag;
	memset(&causediag, 0, sizeof(causediag));
	if (ioctl(sock,SIOCX25GCAUSEDIAG, &causediag) < 0) {
		perror("ioctl(SIOCX25GCAUSEDIAG):");
		return;
	}
	printf("C: %d D: %d\n", causediag.cause, causediag.diagnostic);
}

int connect_x25(char *local_addr, char *remote_addr) {
	// Create an X.25 socket.
	int sock = socket(AF_X25, SOCK_SEQPACKET, 0);
	if (sock < 0) {
		perror("socket:");
		return -1;
	}

	// Set the local address.
	struct sockaddr_x25 laddr;
	memset(&laddr, 0, sizeof(laddr));
	laddr.sx25_family = AF_X25;
	strncpy(laddr.sx25_addr.x25_addr, local_addr, X25_ADDR_LEN);
	if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		perror("bind:");
		return -1;
	}

	struct x25_calluserdata cud;
	memset(&cud, 0, sizeof(cud));
	cud.cudlength = 4;
	cud.cuddata[0] = 0x01;
	cud.cuddata[1] = 0x00;
	cud.cuddata[2] = 0x00;
	cud.cuddata[3] = 0x00;
	if (ioctl(sock, SIOCX25SCALLUSERDATA, &cud) < 0) {
		perror("ioctl(SIOCX25SCALLUSERDATA):");
		return -1;
	}

	// Set the window sizes and packet sizes for the call request.
	struct x25_facilities facilities;
	memset(&facilities, 0, sizeof(facilities));
	facilities.winsize_in = 4;
	facilities.winsize_out = 4;
	facilities.pacsize_in = 9; // 2^9 = 512 bytes
	facilities.pacsize_out = 9;
	if (ioctl(sock, SIOCX25SFACILITIES, &facilities) < 0) {
		perror("ioctl(SIOCX25SFACILITIES):");
		return -1;
	}

	// Connect to remote address.
	struct sockaddr_x25 raddr;
	memset(&raddr, 0, sizeof(raddr));
	raddr.sx25_family = AF_X25;
	strncpy(raddr.sx25_addr.x25_addr, remote_addr, X25_ADDR_LEN);
	if (connect(sock, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
		perror("connect:");
		show_cause_x25(sock);
		return -1;
	}

	return sock;
}

void read_until(int sock, int seconds) {
	while (1) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		struct timeval tv;
		tv.tv_sec = seconds;
		tv.tv_usec = 0;

		int rc = select(sock + 1, &readfds, NULL, NULL, &tv);
		if (rc < 0) {
			perror("select: ");
		} else if (rc) {
			if (FD_ISSET(sock, &readfds)) {
				char buf[1024];
				size_t n = read(sock, buf, sizeof(buf));
				buf[n] = '\0';
				printf("%s", buf);
			}
		} else {
			printf("Timed out waiting for data.");
			break;
		}
	}
}

int main(int argc, char *argv[]) {
	int sock = connect_x25("999888", "701001");
	if (sock < 0) {
		return -1;
	}
	printf("Connected\n");
	describe_x25(sock);
	read_until(sock, 5);
	close(sock);
}
