// LAB 6: Your driver code here

#include <inc/error.h>
#include <inc/string.h>

#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>

// VA del BAR 0
volatile uint8_t *e1000_bar0;

// Array de Transmit Descriptors
static struct tx_desc tx_descriptors[TX_MAX_DESC];

// Array de Packets
static uint8_t tx_packets[TX_MAX_DESC][MAX_PACKET_SIZE];


/*--------------------*/
/* Funciones privadas */
/*--------------------*/

// Obtiene el valor del registro e1000_bar0[offset]
static uint32_t
e1000_getreg(uint32_t offset) {
  return *(volatile uint32_t *) (e1000_bar0 + offset);
}

// Escribe el valor en el registro e1000_bar0[offset]
static void
e1000_setreg(uint32_t offset, uint32_t value)
{
  *(volatile uint32_t *) (e1000_bar0 + offset) = value;
}

// Inicializa los transmit descriptors
void
tx_descriptors_init(void) {
	size_t i;
	for (i = 0; i < TX_MAX_DESC; i ++) {
		tx_descriptors[i].buffer_addr = PADDR(&tx_packets[i]);
		tx_descriptors[i].status |= E1000_TXD_STAT_DD;
	}
}

// Inicializa los registros BAR
void
tx_registers_init(void) {
	// Inicializo los registros Transmit Descriptor Base Address (TDBAL y TDBAH)
	uint64_t array_addr = PADDR(&tx_descriptors);
	uint32_t lower_bits = array_addr;
	uint32_t higher_bits = (array_addr >> 32);
	e1000_setreg(E1000_TDBAL, lower_bits);
	e1000_setreg(E1000_TDBAH, higher_bits);

	// Inicializo el registro Transmit Descriptor Length (TDLEN)
	e1000_setreg(E1000_TDLEN, TX_MAX_DESC * sizeof(struct tx_desc));

	// Inicializo los registros Transmit Descriptor Head y Tail (TDH y TDT)
	e1000_setreg(E1000_TDH, 0);
	e1000_setreg(E1000_TDT, 0);

	// Inicializo el registro Transmit Control (TCTL)
	e1000_setreg(E1000_TCTL, 0);

	// Seteo el Enable Bit (TCTL_EN) y el Pad Short Packets Bit (TCTL_PSP) en 1
	int tctl_flags = E1000_TCTL_EN | E1000_TCTL_PSP;
	e1000_setreg(E1000_TCTL, e1000_bar0[E1000_TCTL] | tctl_flags);

	// Seteo el Collision Threshold (TCTL.CT) en 0x10 (hexa) = 16 (decimal)
	e1000_setreg(E1000_TCTL, e1000_bar0[E1000_TCTL] | (16 << E1000_TCTL_CT));

	// Seteo el Collision Distance (TCTL.COLD) en 0x40 (hexa) = 64 (decimal)
	e1000_setreg(E1000_TCTL, e1000_bar0[E1000_TCTL] | (64 << E1000_TCTL_COLD));

	// Inicializo el registro Transmit Inter Packet Gap (TIPG)
	e1000_setreg(E1000_TIPG, 0);
	// - IPGT (bits 0 a 9) = 10 (decimal)
	e1000_setreg(E1000_TIPG, e1000_bar0[E1000_TIPG] | 10);
	// - IPGR1 (bits 10 a 19) = 8 (decimal)
	e1000_setreg(E1000_TIPG, e1000_bar0[E1000_TIPG] | (8 << 10));
	// - IPGR2 (bits 20 a 29) = 6 (decimal)
	e1000_setreg(E1000_TIPG, e1000_bar0[E1000_TIPG] | (6 << 20));
	// - Reserved (bits 30 a 31) = 0
}

// Inicializa la cola de transmision
void
e1000_init_transmit_queue(void) {
	// Inicializo cada transmit descriptor con el array tx_packets
	tx_descriptors_init();

	// Inicializo los registros
	tx_registers_init();
}


// Transmite un paquete
int
e1000_send_packet(const void *buf, size_t len) {

	int r = 0;

	// Obtengo el indice en la queue dado por el tail register
	uint32_t idx = e1000_getreg(E1000_TDT);

	// Si el DD Bit esta en 1, puedo reciclar el descriptor y usarlo para transmitir el paquete
	bool is_dd_set = (tx_descriptors[idx].status & E1000_TXD_STAT_DD);

	if (is_dd_set){
		// Seteo el RS y el EOP bit del Command Field en 1
		int cmd_flags = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP
		tx_descriptors[idx].cmd |= cmd_flags;
		
		// Seteo el DD Bit del Status en 0, para indicar que esta en uso
		tx_descriptors[idx].status &= ~(1 << E1000_TXD_STAT_DD);
		
		// Para transmitir un paquete, lo agrego al tail (TDT) de la cola de transmision
		// Esto equivale a copiar el paquete en el siguiente buffer
		//memcpy(KADDR(tx_descriptors[idx].buffer_addr), buf, len);
		memcpy(tx_packets[idx], buf, len);
		
		// Actualizo el registro TDT
		idx = (idx + 1) % TX_MAX_DESC;
  		e1000_setreg(E1000_TDT, idx);
	} else {
		// Devuelvo un codigo de error para que el caller de esta funcion
		// sepa que el paquete no se pudo enviar
		r = -E_FULL_TX_QUEUE;
	}

	return r;
}

// Recibe un paquete
int
e1000_receive_packet(void *buffer, size_t bufsize) {
	// ...

	return 0;
}

/*--------------------*/
/* Funciones publicas */
/*--------------------*/

// Inicializa el E1000
int
e1000_attach(struct pci_func *pcif) {
	// Habilito el E1000 device
	pci_func_enable(pcif);

	// Creo un mapeo virtual para el BAR 0 (Base Address Register)
	uint32_t reg_base0 = pcif->reg_base[0];
	uint32_t reg_size0 = pcif->reg_size[0];
	e1000_bar0 = mmio_map_region(reg_base0, reg_size0);

	// Compruebo que el status es el correcto (0x80080783)
	/*
	uint32_t status = e1000_getreg(E1000_STATUS);
	cprintf("0x%x\n", status);
	*/

	// Inicializo la cola de transmision
	e1000_init_transmit_queue();

	// Compruebo que el paquete se transmite correctamente
	e1000_send_packet("Hola", 4);
	e1000_send_packet("Mundo", 5);
	e1000_send_packet("Como", 4);
	e1000_send_packet("Estan?", 6);

	// Inicializo la cola de recepcion
	//e1000_init_receive_queue();

	return 0;
}