#include <WinSock2.h>
#include <iostream>		//cout

#define PAYLOAD_SIZE	16		// Echo Request Data size
#define ICMP_ECHOREQ	8

#pragma pack(1)

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	u_char  VIHL;			// Version and IHL
	u_char	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	u_char	TTL;			// Time To Live
	u_char	Protocol;		// Protocol
	u_short	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	DWORD	dwTime;
	char	cData[PAYLOAD_SIZE];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;

#pragma pack()

//////////////////////////////////////////////////////////////////////////
// prototypes
//////////////////////////////////////////////////////////////////////////

u_short in_cksum(u_short *addr, int len);

int SendEchoRequest(SOCKET s,LPSOCKADDR_IN lpstToAddr);
int WaitForEchoReply(SOCKET s);
DWORD RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL);

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

void PrintWSError(const char* _msg)
{
	std::cout<< _msg << " - WSAError: " << WSAGetLastError() << "\n";
}
