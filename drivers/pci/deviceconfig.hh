#pragma once


namespace pci {

template<typename T>
struct __attribute__((packed)) capability {
	uint16_t NEXT;
	T data;
}; 

struct __attribute__((packed)) MSI_payload {
	uint16_t message_ctl;
	void* message_addr;
	uint16_t data;
};

struct __attribute__((packed)) UNK_payload {};

using MSI = capability<MSI_payload>;
using UNK = capability<UNK_payload>;

};
