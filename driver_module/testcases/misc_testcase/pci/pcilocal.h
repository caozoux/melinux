#ifndef _ME_PCILOCLA_H_
#define _ME_PCILOCLA_H_

//the pci spec config data space
struct pcidevice_config_data 
{
	uint16_t vendor;
	uint16_t device;
	uint16_t command;
	uint16_t status;
	//uint16_t resv;
	uint32_t class_code; //low 8bit is reserver
	uint8_t  cache_line;
	uint8_t  lat_timer;
	uint8_t  header_type;
	uint8_t  bist;
	uint32_t resource0;
	uint32_t resource1;
	uint32_t resource2;
	uint32_t resource3;
	uint32_t resource4;
	uint32_t resource5;
	uint32_t cardbus_pointer;
	uint16_t subvendor;
	uint16_t subid;
	uint32_t rom_baseaddr;
	uint16_t cap_pointer;
	uint16_t reserved_1;
	uint32_t reserved_2;
	uint8_t  interrupt_line;
	uint8_t  interrupt_pin;
	uint8_t  min_gnt;
	uint8_t  max_lat;
};

#endif
