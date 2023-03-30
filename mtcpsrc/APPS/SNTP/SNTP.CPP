/*

   mTCP Sntp.cpp
   Copyright (C) 2010-2022 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP


   This file is part of mTCP.

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.


   Description: Your typical run-of-the-mill SNTP (simple network time
     protocol client).  DOS is kind of limited by the system timer,
     which only has a resolution of 55ms.  So don't go overboard on
     the accuracy because it just doesn't matter.  Getting to within
     a few dozen milliseconds of a public NTP server is more than
     good enough.

   Changes:

   2011-05-27: Initial release as open source software
   2015-01-18: Minor change to Ctrl-Break and Ctrl-C handling.
   2022-01-22: Use the NTP fractional seconds to set the time just
               a little more accurately.

*/





#include <bios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"

#include "trace.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "udp.h"
#include "dns.h"
#include "timer.h"

#include "sntp.h"


#define SERVER_ADDR_NAME_LEN (80)



char     ServerAddrName[ SERVER_ADDR_NAME_LEN ];
IpAddr_t ServerAddr;
uint16_t ServerPort = 123;

uint16_t TimeoutSecs = 3;
uint16_t Retries = 2;
uint8_t  Verbose = 0;
uint8_t  SetTime = 0;


// Set by the NTP UDP handler when a response is received.
time_t   TargetTimeSecs = 0;   // Seconds, Coordinated Universal Time
uint32_t TargetTimeFrac = 0;   // Fractional seconds, NTP format




// Ctrl-Break and Ctrl-C handler.  Check the flag once in a while to see if
// the user wants out.

volatile uint8_t CtrlBreakDetected = 0;

void __interrupt __far ctrlBreakHandler( ) {
  CtrlBreakDetected = 1;
}



typedef struct {
  UdpPacket_t udpHdr;   // Space for Ethernet, IP and UDP headers.
  NTP_packet_t ntp;     // Actual NTP data.
} NTP_UDP_packet_t;

NTP_UDP_packet_t OutgoingRequest;



// Function prototypes

void parseArgs( int argc, char *argv[] );
void shutdown( int rc );

int8_t sendRequest( void );
void ntpUdpHandler( const unsigned char *packet, const UdpHeader *udp );




char printTimeStampBuffer[40];

char * printTimeStamp( uint32_t ts_p, uint32_t frac, bool localTime ) {

  // This uses NTP format, so if you print a DOS timestamp you have to
  // convert it to the NTP format first.

  time_t ts = ts_p;

  struct tm tmbuf;

  if ( localTime ) {
    _localtime( &ts, &tmbuf );
  }
  else {
    _gmtime( &ts, &tmbuf );
  }

  sprintf( printTimeStampBuffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
          tmbuf.tm_year+1900, tmbuf.tm_mon+1, tmbuf.tm_mday,
          tmbuf.tm_hour, tmbuf.tm_min, tmbuf.tm_sec, (frac / 1048) >> 12 );

  return printTimeStampBuffer;

}




int16_t setDosDateTime( void ) {

  struct dosdate_t dos_date;
  struct dostime_t dos_time;


  time_t ts1 = TargetTimeSecs;

  struct tm tmbuf;
  _localtime( &ts1, &tmbuf );

  dos_date.year = tmbuf.tm_year + 1900;
  dos_date.month = tmbuf.tm_mon+1;
  dos_date.day = tmbuf.tm_mday;

  int rc1 = _dos_setdate( &dos_date );

  dos_time.hour = tmbuf.tm_hour;
  dos_time.minute = tmbuf.tm_min;
  dos_time.second = tmbuf.tm_sec;
  dos_time.hsecond = uint32_t((TargetTimeFrac / 10480)) >> 12; // NTP fractional to 1/100ths

  int rc2 = _dos_settime( &dos_time );

  if (rc1 || rc2 ) return 1;

  return 0;
}



static char CopyrightMsg1[] = "mTCP SNTP Client by M Brutman (mbbrutman@gmail.com) (C)opyright 2009-2022\n";
static char CopyrightMsg2[] = "Version: " __DATE__ "\n\n";


int main( int argc, char *argv[] ) {

  int mainRc = 1;

  printf( "%s  %s", CopyrightMsg1, CopyrightMsg2 );

  parseArgs( argc, argv );

  TimeoutSecs = TimeoutSecs * 1000;

  // Initialize TCP/IP
  if ( Utils::parseEnv( ) != 0 ) {
    exit( 1 );
  }


  char *tzStr = getenv( "TZ" );
  if ( tzStr == NULL ) {
    puts( "Warning: the TZ environment variable is not set.  Assuming\n"
          "Eastern Standard Time.  See SNTP.TXT for how to set it properly.\n" );
  }

  tzset( );

  if ( Verbose ) {
    printf( "Timezone name[0]: %s  name[1]: %s\n", tzname[0], tzname[1] );
    printf( "Timezone offset in seconds: %ld\n", timezone );
    printf( "Daylight savings time supported: %d\n\n", daylight );
  }


  // No sockets, no buffers TCP buffers
  if ( Utils::initStack( 0, 0, ctrlBreakHandler, ctrlBreakHandler ) ) {
    puts( "Failed to initialize TCP/IP - exiting" );
    exit( 1 );
  }


  // From this point forward you have to call the shutdown( ) routine to
  // exit because we have the timer interrupt hooked.


  printf( "Resolving %s, press [ESC] to abort.\n", ServerAddrName );

  // Resolve the name and definitely send the request
  int8_t rc = Dns::resolve( ServerAddrName, ServerAddr, 1 );
  if ( rc < 0 ) {
    puts( "Error resolving server" );
    shutdown( 1 );
  }


  uint8_t userEarlyExit = 0;

  clockTicks_t startTime = TIMER_GET_CURRENT( );

  while ( 1 ) {

    if ( CtrlBreakDetected ) {
      puts( "Ctrl-Break detected: aborting\n" );
      userEarlyExit = 1;
      break;
    }

    if ( bioskey(1) != 0 ) {
      char c = bioskey(0);
      if ( (c == 27) || (c == 3) ) {
        puts( "Ctrl-C or ESC detected: aborting\n" );
        userEarlyExit = 1;
        break;
      }
    }

    if ( !Dns::isQueryPending( ) ) break;

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Dns::drivePendingQuery( );

  }

  if ( userEarlyExit ) {
    shutdown( 1 );
  }


  // Query is no longer pending or we bailed out of the loop.
  rc = Dns::resolve( ServerAddrName, ServerAddr, 0 );

  if ( rc != 0 ) {
    puts( "Error resolving server name - exiting" );
    shutdown( 1 );
  }

  uint32_t t = Timer_diff( startTime, TIMER_GET_CURRENT( ) ) * TIMER_TICK_LEN;

  printf( "NTP server ip address is: %d.%d.%d.%d, resolved in %ld.%02ld seconds\n",
           ServerAddr[0], ServerAddr[1], ServerAddr[2], ServerAddr[3],
           (t/1000), (t%1000)
        );


  // Register UDP Handler - should never fail
  Udp::registerCallback( 1024, ntpUdpHandler );


  for ( int i=0; i < Retries; i++ ) {

    if ( Verbose ) {
      printf( "\nSending request # %d\n", i );
    }

    int rc = sendRequest( );

    if ( rc == 1 ) {
      // We timed out ... try again
      continue;
    } else if ( rc == -1 ) {
      puts( "Error: Unable to send UDP packets!" );
      break;
    }
      


    // Spin again until we get a response

    clockTicks_t startTime = TIMER_GET_CURRENT( );

    while ( TargetTimeSecs == 0 ) {

      if ( Timer_diff( startTime, TIMER_GET_CURRENT( ) ) > TIMER_MS_TO_TICKS( TimeoutSecs ) ) {
        TRACE_WARN(( "Sntp: Timeout waiting for sntp response\n" ));
        puts( "Timeout waiting for server response" );
        break;
      }

      PACKET_PROCESS_SINGLE;
      Arp::driveArp( );

    }

    if ( TargetTimeSecs ) break;

  }
  

  if ( TargetTimeSecs && SetTime ) {
    int rc = setDosDateTime( );
    if ( rc == 0 ) {
      printf( "\nSystem time set to new value\n" );
      mainRc = 0;
    }
    else {
      printf( "\nError setting system time!\n" );
    }
  }
  else {
    puts( "\nSystem time has not been updated." );
  }


  shutdown( mainRc );

  // Never get here - we return from shutdown
  return 0;
}



char *HelpText[] = {
  "\nsntp [options] <ipaddr>\n",
  "Options:",
  "  -help          Shows this help",
  "  -port <n>      Contact server on port <n> (default=123)",
  "  -retries <n>   Number of times to retry if no answer (default=1)",
  "  -set           Set the system time (default is not to)",
  "  -timeout <n>   Seconds to wait for a server response (default=3)",
  NULL
};





void usage( void ) {
  uint8_t i=0;
  while ( HelpText[i] != NULL ) {
    puts( HelpText[i] );
    i++;
  }
  exit( 1 );
}



void parseArgs( int argc, char *argv[] ) {

  int i=1;
  for ( ; i<argc; i++ ) {

    if ( argv[i][0] != '-' ) break;

    if ( stricmp( argv[i], "-help" ) == 0 ) {
      usage( );
    }
    else if ( stricmp( argv[i], "-port" ) == 0 ) {
      i++;
      if ( i == argc ) {
        usage( );
      }
      ServerPort = atoi( argv[i] );
      if ( ServerPort == 0 ) {
        puts( "Bad parameter for -port: can not use 0" );
        usage( );
      }
    }
    else if ( stricmp( argv[i], "-retries" ) == 0 ) {
      i++;
      if ( i == argc ) {
        usage( );
      }
      Retries = atoi( argv[i] );
      if ( Retries == 0 ) {
        puts( "Bad parameter for -retries: Should be greater than 0" );
        usage( );
      }
    }
    else if ( stricmp( argv[i], "-set" ) == 0 ) {
      SetTime = 1;
    }
    else if ( stricmp( argv[i], "-timeout" ) == 0 ) {
      i++;
      if ( i == argc ) {
        usage( );
      }
      TimeoutSecs = atoi( argv[i] );
      if ( TimeoutSecs == 0 ) {
        puts( "Bad parameter for -timeout: Should be greater than 0" );
        usage( );
      }
    }
    else if ( stricmp( argv[i], "-verbose" ) == 0 ) {
      Verbose = 1;
    }
    else {
      printf( "Unknown option %s\n", argv[i] );
      usage( );
    }

  }

  if ( i == argc ) {
    puts( "You need to specify a machine name or IP address" );
    usage( );
  }

  strncpy( ServerAddrName, argv[i], SERVER_ADDR_NAME_LEN );
  ServerAddrName[ SERVER_ADDR_NAME_LEN - 1 ] = 0;

}



void shutdown( int rc ) {
  Utils::endStack( );
  exit( rc );
}




int8_t sendRequest( void ) {

  // Setup outgoing packet
  memset( &OutgoingRequest, 0, sizeof(OutgoingRequest) );

  // Get the current time.  We use time() to get Coordinated Universal Time
  // and _dos_gettime to get the fraction of a second.  They are both based
  // on the same time, just the timezone offset is different between them.
  // (_dos_gettime is local time.)

  struct dostime_t dos_time;
  _dos_gettime( &dos_time );

  OutgoingRequest.ntp.leapIndicator = 3;  // Unknown or unset
  OutgoingRequest.ntp.version = 3;        // Version = 3
  OutgoingRequest.ntp.mode = 3;           // Mode = Client

  OutgoingRequest.ntp.transTimeSecs = time( NULL );
  OutgoingRequest.ntp.transTimeFrac = (uint32_t(dos_time.hsecond) * 10480) << 12;

  if ( Verbose ) {
    printf( "Outgoing transmit time: %s\n",
            printTimeStamp( OutgoingRequest.ntp.transTimeSecs, OutgoingRequest.ntp.transTimeFrac, false ) );
  }

  OutgoingRequest.ntp.transTimeSecs = htonl( OutgoingRequest.ntp.transTimeSecs +  NTP_OFFSET );
  OutgoingRequest.ntp.transTimeFrac = htonl( OutgoingRequest.ntp.transTimeFrac );

  uint16_t reqLen = sizeof( OutgoingRequest.ntp );

  clockTicks_t startTime = TIMER_GET_CURRENT( );

  int rc = Udp::sendUdp( ServerAddr, 1024, ServerPort, reqLen,
                         (uint8_t *)&OutgoingRequest, 1 );

  if ( rc == -1 ) return -1;

  // Spin and process packets until we can resolve ARP and send our request
  while ( rc == 1 ) {

    if ( Timer_diff( startTime, TIMER_GET_CURRENT( ) ) > TIMER_MS_TO_TICKS( 2000 ) ) {
      TRACE_WARN(( "Sntp: Arp timeout sending request\n" ));
      puts( "Warning: ARP timeout sending request - check your gateway setting" );
      return -1;
    }

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );

    rc = Udp::sendUdp( ServerAddr, 1024, ServerPort, reqLen,
                       (uint8_t *)&OutgoingRequest, 1 );

    if ( rc == -1 ) return -1;

  }

  return 0;

}




void ntpUdpHandler( const unsigned char *packet, const UdpHeader *udp ) {

  NTP_packet_t *ntp = &(((NTP_UDP_packet_t *)(packet))->ntp);

  // Quick sanity check

  if ( (ntohs(udp->src) == ServerPort) && (ntohs(udp->dst) == 1024) && ( (ntp->mode & 0x7) == 4 ) ) {

    if ( Verbose ) {

      printf( "\nReponse packet from ntp server: \n" );
      printf( "  Leap indicator: %d\n", (ntp->mode & 0xc0) >> 6 );
      printf( "  Version number: %d\n", (ntp->mode & 0x38) >> 3 );
      printf( "  Stratum:        %d\n", ntp->stratum );
      printf( "  Reference ts:   %s UTC\n", printTimeStamp( ntohl( ntp->refTimeSecs ) - NTP_OFFSET, ntohl(ntp->refTimeFrac), false ) );
      printf( "  Original ts:    %s UTC\n", printTimeStamp( ntohl( ntp->origTimeSecs ) - NTP_OFFSET, ntohl(ntp->origTimeFrac), false ) );
      printf( "  Receive ts:     %s UTC\n", printTimeStamp( ntohl( ntp->recvTimeSecs ) - NTP_OFFSET, ntohl(ntp->recvTimeFrac), false ) );
      printf( "  Transmit ts:    %s UTC\n", printTimeStamp( ntohl( ntp->transTimeSecs ) - NTP_OFFSET, ntohl(ntp->transTimeFrac), false ) );

    }


    // What we are doing here is taking the transmit timestamp from the
    // server and using it directly, which is wrong.  We are supposed
    // to generate a new local timestamp (Destination Timestamp) and then
    // compute a few deltas to arrive at the adjustment that we need.
    //
    // This is DOS. We are going to use what they gave us directly since
    // we know the standard timer resolution on the machine is at best
    // 55ms.
 
    TargetTimeSecs = ntohl( ntp->transTimeSecs ) - NTP_OFFSET;
    TargetTimeFrac = ntohl( ntp->transTimeFrac );


    // Difference calculation.

    // Convert NTP fraction to milliseconds
    uint16_t targetFrac = uint16_t((TargetTimeFrac / 1048) >> 12);


    struct dostime_t dos_time;
    _dos_gettime( &dos_time );

    time_t   currentTime = time( NULL );
    uint16_t currentFrac = dos_time.hsecond * 10;

    time_t   diffSecs;
    uint16_t diffFrac = 0;


    if ( currentTime > TargetTimeSecs ) {

      diffSecs = currentTime - TargetTimeSecs;

      if ( currentFrac > targetFrac ) {
        diffFrac = currentFrac - targetFrac;
      } else {
        diffSecs--;
        diffFrac = (1000 + currentFrac) - targetFrac;
      }

    } else if ( TargetTimeSecs > currentTime ) {

      diffSecs = TargetTimeSecs - currentTime;

      if ( targetFrac > currentFrac ) {
        diffFrac = targetFrac - currentFrac;
      } else {
        diffSecs--;
        diffFrac = (1000 + targetFrac) - currentFrac;
      }

    } else {

      diffSecs = 0;

      if ( currentFrac > targetFrac ) {
        diffFrac = currentFrac - targetFrac;
      } else {
        diffFrac = targetFrac - currentFrac;
      }

    }



    printf( "\nYour selected timezone is: %s\n", tzname[0] );

    printf( "\nCurrent system time is: %s\n", printTimeStamp( currentTime, (uint32_t(dos_time.hsecond) * 10480) << 12 , true ) );
    printf( "Time should be set to:  %s\n\n", printTimeStamp(  TargetTimeSecs, TargetTimeFrac, true ) );

    if ( ((diffSecs * 1000) + diffFrac) < 600000 ) {
      printf( "Difference between suggested time and system time is: %lu.%03u seconds\n", diffSecs, diffFrac );
    }
    else {
      puts( "Difference between suggested time and system time is greater than 10 minutes!" );
    }

    puts( "(Remember, the smallest increment of time for DOS is 55 milliseconds.)" );


  }

  // We are done processing this packet.  Remove it from the front of
  // the queue and put it back on the free list.
  Buffer_free( packet );

}

