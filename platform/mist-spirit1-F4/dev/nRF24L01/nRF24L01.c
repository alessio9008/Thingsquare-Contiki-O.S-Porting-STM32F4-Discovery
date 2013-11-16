#include "spi.h"
#include "nRF24L01.h"
#include "stm32f10x_lib.h"
#include "secondarylib.h"
#include "stepmotor.h"
#include "usart.h"
#include "timer.h"
u8 TxBuf[32]={0};
volatile unsigned int sta;
//bool sendOnce;

unsigned char TX_ADDRESS[TX_ADR_WIDTH]  = {0x34,0x43,0x10,0x10,0x01}; // Define a static TX address
unsigned char rx_buf[TX_PLOAD_WIDTH];
unsigned char tx_buf[TX_PLOAD_WIDTH];
unsigned char flag;
unsigned char rx_com_buffer[10];
unsigned char tx_com_buffer[10];
unsigned char i;
unsigned char accept_flag;
// SPI(nRF24L01) commands

#define RF_READ_REG    0x00  // Define read command to register
#define RF_WRITE_REG   0x20  // Define write command to register
#define RD_RX_PLOAD 0x61  // Define RX payload register address
#define WR_TX_PLOAD 0xA0  // Define TX payload register address
#define FLUSH_TX    0xE1  // Define flush TX register command
#define FLUSH_RX    0xE2  // Define flush RX register command
#define REUSE_TX_PL 0xE3  // Define reuse TX payload register command
#define NOP         0xFF  // Define No Operation, might be used to read status register

#define  RX_DR  ((sta>>6)&0X01)
#define  TX_DS  ((sta>>5)&0X01)
#define  MAX_RT  ((sta>>4)&0X01)

//Chip Enable Activates RX or TX mode
#define CE_H()   GPIO_SetBits(GPIOE, GPIO_Pin_1) 
#define CE_L()   GPIO_ResetBits(GPIOE, GPIO_Pin_1)

//SPI Chip Select
#define SPI2_releaseChip()  GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define SPI2_selectChip()  GPIO_ResetBits(GPIOB, GPIO_Pin_12)

void SPI2_Init(void)
{
	SPI_InitTypeDef   SPI_InitStructure;
  GPIO_InitTypeDef 	GPIO_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); //enable spi2 clock.
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;// |GPIO_Pin_14 |GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 
	//CE
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
	//IRQ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

	SPI_Cmd(SPI2, DISABLE); 
	/* SPI2 configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master; 
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;  
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; 
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;// 
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; 
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; 
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI2, &SPI_InitStructure);
  /* Enable SPI2   */
  SPI_Cmd(SPI2, ENABLE);	
}

void Delay_us(unsigned int  n)
{
	u32 i;
	
	while(n--)
	{
 	   i=2;
 	   while(i--);
  }
}



/*******************************************************************************
* Function Name   : SPI2_readWriteByte
* Description 		: Sends a byte through the SPI interface and return the byte
*                		received from the SPI bus.
* Input       		: byte : byte to send.
* Output       		: None
* Return       		: The value of the received byte.
*******************************************************************************/
unsigned char SPI2_readWriteByte(unsigned char byte)
{
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	/* Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(SPI2, byte);
	/* Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI2);
}

/*******************************************************************************
* Function Name   : SPI2_writeReg
* Description 		: write a byte into the register specified and return the 
										status byte.
* Input       		: value: value to be written.	reg:register to write to.
* Output       		: None
* Return       		: The status byte.
*******************************************************************************/

unsigned char SPI2_writeReg(unsigned char reg, unsigned char value)
{
	unsigned char status;
	SPI2_selectChip();
	//select register 
	status = SPI2_readWriteByte(reg);
	//write to it the value
	SPI2_readWriteByte(value);
	SPI2_releaseChip();
	return(status);
}

/*******************************************************************************
* Function Name   : SPI2_readReg
* Description 		: read from a specified register.
* Input       		: reg:register to read from.
* Output       		: None
* Return       		: The correponding value.
*******************************************************************************/

unsigned char SPI2_readReg(unsigned char reg)
{
	unsigned char reg_val;
	SPI2_selectChip();                    
	SPI2_readWriteByte(reg);                
	reg_val = SPI2_readWriteByte(0);       
	SPI2_releaseChip();                    
	return(reg_val);            
}

unsigned char SPI2_readBuf(unsigned char reg,unsigned char *pBuf, unsigned char bufSize)
{
	unsigned char status,i;
	SPI2_selectChip();
	// Select register to write to and read status byte
	status = SPI2_readWriteByte(reg);
	for(i=0;i<bufSize;i++)
		pBuf[i] = SPI2_readWriteByte(0);
	SPI2_releaseChip();
	return(status);
}

unsigned char SPI2_writeBuf(unsigned char reg, unsigned char *pBuf, unsigned char bufSize)
{
	unsigned char status,i;
	SPI2_selectChip();
	// Select register to write to and read status byte
	status = SPI2_readWriteByte(reg);
	for(i=0; i<bufSize; i++) // then write all byte in buffer(*pBuf)
		SPI2_readWriteByte(*pBuf++);
	SPI2_releaseChip();
	return(status);
}

void RX_Mode(void)
{
	 CE_L();
	SPI2_writeBuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);
	SPI2_writeReg(WRITE_REG + EN_AA, 0x01);
	// Enable Auto.Ack:Pipe0
	SPI2_writeReg(WRITE_REG + EN_RXADDR, 0x01); // Enable Pipe0
	SPI2_writeReg(WRITE_REG + RF_CH, 40);
	// Select RF channel 40
	SPI2_writeReg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH);
	SPI2_writeReg(WRITE_REG + RF_SETUP, 0x07);
	SPI2_writeReg(WRITE_REG + CONFIG, 0x0f);
	// Set PWR_UP bit, enable CRC(2 bytes)
	//& Prim:RX. RX_DR enabled..
	 CE_H(); // Set CE pin high to enable RX device
	// This device is now ready to receive one packet of 16 bytes payload from a TX device
	//sending to address
	// '3443101001', with auto acknowledgment, retransmit count of 10, RF channel 40 and
	//datarate = 2Mbps.
}

void TX_Mode(unsigned char * BUF)
{
	GPIO_ResetBits(GPIOE,GPIO_Pin_1);//CE=0
	SPI2_writeBuf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
	SPI2_writeBuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);
	SPI2_writeBuf(WR_TX_PLOAD, BUF, TX_PLOAD_WIDTH); // Writes data to TX payload
	SPI2_writeReg(WRITE_REG + EN_AA, 0x01);
	// Enable Auto.Ack:Pipe0
	SPI2_writeReg(WRITE_REG + EN_RXADDR, 0x01); // Enable Pipe0
	SPI2_writeReg(WRITE_REG + SETUP_RETR, 0x1a); // 500us + 86us, 10 retrans...
	SPI2_writeReg(WRITE_REG + RF_CH, 40);
	// Select RF channel 40
	SPI2_writeReg(WRITE_REG + RF_SETUP, 0x07);
	// TX_PWR:0dBm, Datarate:2Mbps,
//	LNA:HCURR
	SPI2_writeReg(WRITE_REG + CONFIG, 0x0e);
	// Set PWR_UP bit, enable CRC(2 bytes)
	//& Prim:TX. MAX_RT & TX_DS enabled..
	GPIO_SetBits(GPIOE,GPIO_Pin_1); // Set CE pin high 
}

//Chip Enable Activates RX or TX mode
#define CE_H()   GPIO_SetBits(GPIOE, GPIO_Pin_1) 
#define CE_L()   GPIO_ResetBits(GPIOE, GPIO_Pin_1)

//SPI Chip Select
#define SPI2_releaseChip()  GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define SPI2_selectChip()  GPIO_ResetBits(GPIOB, GPIO_Pin_12)
void nRF24L01_Enable(void)
{
	CE_L();			// chip enable
	SPI2_releaseChip();			// Spi disable	
	//SCK=0;			// Spi clock line init high
}

void clearFlag(void)
{	 
	SPI2_writeReg(WRITE_REG+STATUS,0xff);	
}

void nRF24L01_ISR(void)
{
	int i;
	sta=SPI2_readReg(STATUS);
	SPI2_writeReg(WRITE_REG+STATUS,0xff);
	if(RX_DR)				
	{
		SPI2_readBuf(RD_RX_PLOAD,rx_buf,TX_PLOAD_WIDTH);
		Serial_PutString(rx_buf);				 //put whatever received to the UART
  }
	if(MAX_RT)
	{
		SPI2_writeReg(FLUSH_TX,0);
  }

	
	RX_Mode();	
}



int main(void)
{
	unsigned int nCount;
	unsigned char vEncoder[20]="--------------------";
	int i;
  RCC_Configuration();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	NVIC_Configuration();
		EXTI_Configuration();
  USART1_Init();
	USART2_Init();															
	USART3_Init();
	UART4_Init();
	UART5_Init();
	SPI2_Init();
	SysTick_Init();
	//init_NRF24L01();
	RX_Mode();
	//nRF24L01_ISR();
	
  while(1)
{

	//Serial_PutString("While ");


}
}	 









