#include <libriscv/machine.hpp>
#include <chrono>
#include <fstream>
#include <iterator>
#define TIME_POINT(t) \
	asm("" ::: "memory"); \
	auto t = std::chrono::high_resolution_clock::now(); \
	asm("" ::: "memory");
namespace {
	extern const std::array<unsigned char, 944> fib_elf;
}

int main(int argc, char **argv)
{
	// Default: fib(256000000)
	std::string_view binview{(const char *)fib_elf.data(), fib_elf.size() };
	std::vector<char> binary;

	// Program argument: Read and execute the provided file
	if (argc > 1) {
		const char* filename = argv[1];

		std::ifstream fs(filename, std::ios::binary);
		if (!fs) {
			fprintf(stderr, "Not able to access %s!\n", filename);
		}
		binary.assign(std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

		binview = { binary.data(), binary.size() };
	}

	// Setup a machine for Linux emulation
	riscv::Machine<riscv::RISCV64> machine{ binview };
	machine.setup_linux_syscalls();
	machine.setup_posix_threads();
	machine.setup_linux({
			"libriscv", "Hello", "World"
		}, {
			"LC_ALL=C", "USER=groot"
		});

	TIME_POINT(t0);
	machine.simulate();
	TIME_POINT(t1);

	const std::chrono::duration<double, std::milli> exec_time = t1 - t0;
	printf("\nRuntime: %.3fms  MI/s: %.2f\n", exec_time.count(),
		machine.instruction_counter() / (exec_time.count() * 1e3));
}

namespace {
	/** 'fib' calculates the 256000000th fibonacci number.
00010074 <_start>:
_start():
   10144:	1141                	add	sp,sp,-16
   10146:	0f4247b7          		lui	a5,0xf424
   1014a:	e43e                	sd	a5,8(sp)
   1014c:	67a2                	ld	a5,8(sp)
   1014e:	4685                	li	a3,1
   10150:	4701                	li	a4,0
   10152:	e399                	bnez	a5,0x10158
   10154:	a829                	j	0x1016e
   10156:	872a                	mv	a4,a0
   10158:	17fd                	add	a5,a5,-1 # 0xf423fff
   1015a:	00d70533          		add	a0,a4,a3
   1015e:	86ba                	mv	a3,a4
   10160:	fbfd                	bnez	a5,0x10156
   10162:	05d00893          		li	a7,93
   10166:	00000073          		ecall
   1016a:	0141                	add	sp,sp,16
   1016c:	8082                	ret
   1016e:	4501                	li	a0,0
   10170:	05d00893          		li	a7,93
   10174:	00000073          		ecall
   10178:	0141                	add	sp,sp,16
   1017a:	8082                	ret
	**/
	static constexpr std::array<unsigned char, 944> fib_elf {
		0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xf3, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x44, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x05, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0x04, 0x00, 0x40, 0x00,
		0x06, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00, 0x70, 0x04, 0x00, 0x00, 0x00,
		0xa7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x7c, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x04, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x20, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x51, 0xe5, 0x74, 0x64, 0x06, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x47, 0x4e, 0x55, 0x00, 0xa6, 0x97, 0x3b, 0xaa, 0x2a, 0x31, 0xff, 0xc1,
		0xfc, 0xfa, 0xf7, 0x55, 0x36, 0x90, 0xd9, 0x25, 0x4b, 0x2f, 0x01, 0xb5,
		0x41, 0x11, 0xb7, 0x47, 0x42, 0x0f, 0x3e, 0xe4, 0xa2, 0x67, 0x85, 0x46,
		0x01, 0x47, 0x99, 0xe3, 0x29, 0xa8, 0x2a, 0x87, 0xfd, 0x17, 0x33, 0x05,
		0xd7, 0x00, 0xba, 0x86, 0xfd, 0xfb, 0x93, 0x08, 0xd0, 0x05, 0x73, 0x00,
		0x00, 0x00, 0x41, 0x01, 0x82, 0x80, 0x01, 0x45, 0x93, 0x08, 0xd0, 0x05,
		0x73, 0x00, 0x00, 0x00, 0x41, 0x01, 0x82, 0x80, 0x47, 0x43, 0x43, 0x3a,
		0x20, 0x28, 0x55, 0x62, 0x75, 0x6e, 0x74, 0x75, 0x20, 0x31, 0x32, 0x2e,
		0x33, 0x2e, 0x30, 0x2d, 0x31, 0x75, 0x62, 0x75, 0x6e, 0x74, 0x75, 0x31,
		0x7e, 0x32, 0x32, 0x2e, 0x30, 0x34, 0x29, 0x20, 0x31, 0x32, 0x2e, 0x33,
		0x2e, 0x30, 0x00, 0x41, 0x49, 0x00, 0x00, 0x00, 0x72, 0x69, 0x73, 0x63,
		0x76, 0x00, 0x01, 0x3f, 0x00, 0x00, 0x00, 0x04, 0x10, 0x05, 0x72, 0x76,
		0x36, 0x34, 0x69, 0x32, 0x70, 0x31, 0x5f, 0x6d, 0x32, 0x70, 0x30, 0x5f,
		0x61, 0x32, 0x70, 0x31, 0x5f, 0x66, 0x32, 0x70, 0x32, 0x5f, 0x64, 0x32,
		0x70, 0x32, 0x5f, 0x63, 0x32, 0x70, 0x30, 0x5f, 0x7a, 0x69, 0x63, 0x73,
		0x72, 0x32, 0x70, 0x30, 0x5f, 0x7a, 0x69, 0x66, 0x65, 0x6e, 0x63, 0x65,
		0x69, 0x32, 0x70, 0x30, 0x00, 0x00, 0x2e, 0x73, 0x68, 0x73, 0x74, 0x72,
		0x74, 0x61, 0x62, 0x00, 0x2e, 0x6e, 0x6f, 0x74, 0x65, 0x2e, 0x67, 0x6e,
		0x75, 0x2e, 0x62, 0x75, 0x69, 0x6c, 0x64, 0x2d, 0x69, 0x64, 0x00, 0x2e,
		0x74, 0x65, 0x78, 0x74, 0x00, 0x2e, 0x63, 0x6f, 0x6d, 0x6d, 0x65, 0x6e,
		0x74, 0x00, 0x2e, 0x72, 0x69, 0x73, 0x63, 0x76, 0x2e, 0x61, 0x74, 0x74,
		0x72, 0x69, 0x62, 0x75, 0x74, 0x65, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x0b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x01, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x44, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x2d, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xa7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xf1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
}