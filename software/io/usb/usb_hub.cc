#include <string.h>
#include <stdlib.h>
extern "C" {
	#include "itu.h"
}
#include "integer.h"
#include "usb_hub.h"
#include "small_printf.h"
#include "event.h"

__inline DWORD cpu_to_32le(DWORD a)
{
    DWORD m1, m2;
    m1 = (a & 0x00FF0000) >> 8;
    m2 = (a & 0x0000FF00) << 8;
    return (a >> 24) | (a << 24) | m1 | m2;
}

/*********************************************************************
/ The driver is the "bridge" between the system and the device
/ and necessary system objects
/*********************************************************************/
// tester instance
UsbHubDriver usb_hub_driver_tester(usb_drivers);

UsbHubDriver :: UsbHubDriver(IndexedList<UsbDriver*> &list)
{
	list.append(this); // add tester
}

UsbHubDriver :: UsbHubDriver()
{
}

UsbHubDriver :: ~UsbHubDriver()
{

}

UsbHubDriver *UsbHubDriver :: create_instance(void)
{
	return new UsbHubDriver();
}

bool UsbHubDriver :: test_driver(UsbDevice *dev)
{
	printf("** Test UsbHubDriver **\n");
	printf("Dev class: %d\n", dev->device_descr.device_class);
	if(dev->device_descr.device_class != 0x09) {
		printf("Device is not a hub..\n");
		return false;
	}
	if((dev->device_descr.protocol != 0x01)&&(dev->device_descr.protocol != 0x02)) {
		printf("Device protocol: no TT's. [%b]\n", dev->device_descr.protocol);
		return false;
	}
	if(dev->interface_descr.sub_class != 0x00) {
		printf("SubClass is not zero... [%b]\n", dev->interface_descr.sub_class);
		return false;
	}

//	if(dev->interface_descr.protocol != 0x50) {
//		printf("Protocol is not bulk only. [%b]\n", dev->interface_descr.protocol);
//		return false;
//	}

	// TODO: More tests needed here?
	return true;
}

BYTE c_get_hub_descriptor[] = { 0xA0, 0x06, 0x00, 0x29, 0x00, 0x00, 0x40, 0x00 };
BYTE c_get_hub_status[]     = { 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 };
BYTE c_get_port_status[]    = { 0xA3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 };
BYTE c_set_hub_feature[]    = { 0x20, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
BYTE c_set_port_feature[]   = { 0x23, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
BYTE c_clear_hub_feature[]  = { 0x20, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
BYTE c_clear_port_feature[] = { 0x23, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define PORT_CONNECTION       0x00
#define PORT_ENABLE           0x01
#define PORT_SUSPEND          0x02
#define PORT_OVER_CURRENT     0x03
#define PORT_RESET            0x04
#define PORT_POWER            0x08
#define PORT_LOW_SPEED        0x09

#define C_PORT_CONNECTION     0x10
#define C_PORT_ENABLE         0x11
#define C_PORT_SUSPEND        0x12
#define C_PORT_OVER_CURRENT   0x13
#define C_PORT_RESET          0x14

#define PORT_TEST             0x15
#define PORT_INDICATOR        0x16

#define BIT_PORT_CONNECTION   0x01
#define BIT_PORT_ENABLE       0x02
#define BIT_PORT_SUSPEND      0x04
#define BIT_PORT_OVER_CURRENT 0x08
#define BIT_PORT_RESET        0x10

void UsbHubDriver :: install(UsbDevice *dev)
{
	printf("Installing '%s %s'\n", dev->manufacturer, dev->product);
    host = dev->host;
    device = dev;
    
	dev->set_configuration(dev->device_config.config_value);

    BYTE *buf;
    
	printf("Getting hub descriptor...\n");
	int i = dev->host->control_exchange(dev->current_address,
								   c_get_hub_descriptor, 8,
								   NULL, 64, &buf);

	printf("Hub descriptor: (returned %d bytes)\n", i);
    for(int j=0;j<i;j++) {
        printf("%b ", buf[j]);
    }
    printf("\n");

    num_ports = int(buf[2]);
    individual_power_switch = ((buf[3] & 0x03) == 1);
    individual_overcurrent  = ((buf[3] & 0x18) == 8);
    compound                = ((buf[3] & 0x04) == 4);
    power_on_time           = 2 * int(buf[5]);
    hub_current             =     int(buf[6]);
    
    printf("Found a hub with %d ports, ", num_ports);
    if(individual_power_switch)
        printf("with individual power switch, ");
    if(individual_overcurrent)
        printf("with individual overcurrent protection, ");
    printf("which is %spart of a compound device.\n", compound?"":"not ");
    printf("The power-good time is %d ms, and the hub draws %d mA.\n", power_on_time, hub_current);

	i = dev->host->control_exchange(dev->current_address,
								   c_get_hub_status, 8,
								   NULL, 64, &buf);

	printf("Hub status: (returned %d bytes)\n", i);
    for(int j=0;j<i;j++) {
        printf("%b ", buf[j]);
    }

    printf("Turning on power for each port..\n");
    // Turn on port power for each port
    c_set_port_feature[2] = PORT_POWER;
    for(int j=0;j<num_ports;j++) {
        c_set_port_feature[4] = BYTE(j+1);
    	i = dev->host->control_exchange(dev->current_address,
    								    c_set_port_feature, 8,
    								    NULL, 64, &buf);
    }
    wait_ms(power_on_time);        

    printf("Reset all ports and getting their status..\n");
    // Reset all ports
    c_set_port_feature[2] = PORT_RESET;
    for(int j=0;j<num_ports;j++) {
        c_set_port_feature[4] = BYTE(j+1);
    	i = dev->host->control_exchange(dev->current_address,
    								    c_set_port_feature, 8,
    								    NULL, 64, &buf);
        wait_ms(power_on_time);

        c_get_port_status[4] = BYTE(j+1);
    	i = dev->host->control_exchange(dev->current_address,
    								    c_get_port_status, 8,
    								    NULL, 64, &buf);
        if(i == 4) {
        	printf("Port %d status:", j);
            for(int b=0;b<4;b++) {
                printf("%b ", buf[b]);
            } printf("\n");
        } else {
            printf("Failed to read port %d status. Got %d bytes.\n", j, i);
            continue;
        }

        if(buf[0] & 0x02) { // PORT_ENABLE set
            UsbDevice *d = new UsbDevice(dev->host);
            d->parent = dev;
            if(!dev->host->install_device(d, false)) {  // false = assume powered hub for now            
                printf("Install failed...\n");
                continue;
            }
        }
    }
    printf("-- Install Complete.. --\n");

    irq_transaction = host->allocate_transaction(1);
    host->interrupt_in(irq_transaction, device->pipe_numbers[0], 1, irq_data);
}

void UsbHubDriver :: deinstall(UsbDevice *dev)
{
    host->free_transaction(irq_transaction);
}

void UsbHubDriver :: poll(void)
{
    int resp = host->interrupt_in(irq_transaction, device->pipe_numbers[0], 1, irq_data);
    if(resp) {
        printf("HUB IRQ data: ");
        for(int i=0;i<resp;i++) {
            printf("%b ", irq_data[i]);
        } printf("\n");
    }
    if(!irq_data[0])
        return;

    BYTE *buf;
    int i;
    
    for(int j=0;j<=num_ports;j++) {
        irq_data[0] >>=1;
        if(irq_data[0] & 1) {
            c_get_port_status[4] = BYTE(j+1);
        	i = host->control_exchange(device->current_address,
        								    c_get_port_status, 8,
        								    NULL, 64, &buf);
            if(i == 4) {
            	printf("Port %d status:", j);
                for(int b=0;b<4;b++) {
                    printf("%b ", buf[b]);
                } printf("\n");

                c_clear_port_feature[4] = BYTE(j+1);

                if(buf[2] & BIT_PORT_CONNECTION) {
                    printf("* CONNECTION CHANGE *\n");
                    c_clear_port_feature[2] = C_PORT_CONNECTION;
                	i = host->control_exchange(device->current_address,
                							   c_clear_port_feature, 8,
                							   NULL, 8, NULL);
                }
                if(buf[2] & BIT_PORT_ENABLE) {
                    printf("* ENABLE CHANGE *\n");
                    c_clear_port_feature[2] = C_PORT_ENABLE;
                	i = host->control_exchange(device->current_address,
                							   c_clear_port_feature, 8,
                							   NULL, 8, NULL);
                }
                if(buf[2] & BIT_PORT_SUSPEND) {
                    printf("* SUSPEND CHANGE *\n");
                    c_clear_port_feature[2] = C_PORT_SUSPEND;
                	i = host->control_exchange(device->current_address,
                							   c_clear_port_feature, 8,
                							   NULL, 8, NULL);
                }
                if(buf[2] & BIT_PORT_OVER_CURRENT) {
                    printf("* OVERCURRENT CHANGE *\n");
                    c_clear_port_feature[2] = C_PORT_OVER_CURRENT;
                	i = host->control_exchange(device->current_address,
                							   c_clear_port_feature, 8,
                							   NULL, 8, NULL);
                }
                if(buf[2] & BIT_PORT_RESET) {
                    printf("* RESET CHANGE *\n");
                    c_clear_port_feature[2] = C_PORT_RESET;
                	i = host->control_exchange(device->current_address,
                							   c_clear_port_feature, 8,
                							   NULL, 8, NULL);
                }
            } else {
                printf("Failed to read port %d status. Got %d bytes.\n", j, i);
                continue;
            }
        }        
    }
}
	
