#include "main.h"

//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
u_short in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

int SendEchoRequest(SOCKET s,LPSOCKADDR_IN lpstToAddr) 
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// Fill in echo request
	echoReq.icmpHdr.Type		= ICMP_ECHOREQ;
	echoReq.icmpHdr.Code		= 0;
	echoReq.icmpHdr.Checksum	= 0;
	echoReq.icmpHdr.ID			= nId++;
	echoReq.icmpHdr.Seq			= nSeq++;

	// Fill in some data to send
	for (nRet = 0; nRet < PAYLOAD_SIZE; nRet++)
		echoReq.cData[nRet] = ' '+nRet;

	// Save tick count when sent
	echoReq.dwTime				= GetTickCount();

	// Put data in packet and compute checksum
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// Send the echo request  								  
	nRet = sendto(s,						/* socket */
		(LPSTR)&echoReq,			/* buffer */
		sizeof(ECHOREQUEST),
		0,							/* flags */
		(LPSOCKADDR)lpstToAddr, /* destination */
		sizeof(SOCKADDR_IN));   /* address length */

	if (nRet == SOCKET_ERROR) 
		PrintWSError("sendto()");

	return (nRet);
}

int WaitForEchoReply(SOCKET s)
{
	struct timeval Timeout;
	fd_set readfds;

	readfds.fd_count = 1;
	readfds.fd_array[0] = s;
	Timeout.tv_sec = 1;
	Timeout.tv_usec = 0;

	return(select(1, &readfds, NULL, NULL, &Timeout));
}

DWORD RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL) 
{
	ECHOREPLY echoReply;
	int nRet;
	int nAddrLen = sizeof(struct sockaddr_in);

	// Receive the echo reply	
	nRet = recvfrom(s,			// socket
		(LPSTR)&echoReply,		// buffer
		sizeof(ECHOREPLY),		// size of buffer
		0,						// flags
		(LPSOCKADDR)lpsaFrom,	// From address
		&nAddrLen);				// pointer to address len

	// Check return value
	if (nRet == SOCKET_ERROR) 
		PrintWSError("recvfrom()");

	// return time sent and IP TTL
	*pTTL = echoReply.ipHdr.TTL;

	return(echoReply.echoRequest.dwTime);   		
}

int main()
{	
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	const char* pstrHost = "www.heise.de";

	// Create a Raw socket
	SOCKET rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rawSocket == SOCKET_ERROR) 
	{
		PrintWSError("socket()");
		return -1;
	}

	// Lookup host
	LPHOSTENT lpHost = gethostbyname(pstrHost);
	if (lpHost == NULL)
	{
		PrintWSError("Host not found");
		return -1;
	}

	// Setup destination socket address
	sockaddr_in saDest;
	saDest.sin_addr.s_addr = *((u_long FAR *) (lpHost->h_addr));
	saDest.sin_family = AF_INET;
	saDest.sin_port = 0;

	for(int i=0; i<5; i++)
	{
		SendEchoRequest(rawSocket, &saDest);

		int nRet = WaitForEchoReply(rawSocket);

		if (nRet == SOCKET_ERROR)
		{
			PrintWSError("select()");
			break;
		}
		if (!nRet)
		{
			PrintWSError("Request Timed Out");
		}
		else
		{
			sockaddr_in	saSrc;
			u_char		cTTL;

			// Receive reply
			DWORD dwTimeSent = RecvEchoReply(rawSocket, &saSrc, &cTTL);

			// Calculate elapsed time
			DWORD dwElapsed = ::GetTickCount() - dwTimeSent;

			std::cout 
				<<"Reply [" << i+1 <<"]"
				<<" from: "<<inet_ntoa(saSrc.sin_addr)<<":"
				<<" bytes='"<<PAYLOAD_SIZE<<"'"
				<<" time="<<dwElapsed<<"ms"
				<<" TTL="<<(int)cTTL
				<<"\n";
				
			::Sleep(1000);
		}
	}

	return 0;
}
