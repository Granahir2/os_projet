#include "user_libc.hh"

extern "C" {

int main() {
	const char* str = "Hello from C userspace!\n";
	fwrite(str, 1, 24, stdout);
	return 0;
}

}
