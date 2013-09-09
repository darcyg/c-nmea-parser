// NMEAParser.cpp: implementation of the NMEAParser class.
//
//////////////////////////////////////////////////////////////////////
#include "NMEAParser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
NMEAParser::NMEAParser()
{
//	 m_GPSInfo.m_dwCommandCount = 0;
}
//---------------------------------------------------------------------------
NMEAParser::NMEAParser(LPCTSTR outputFileName)
{
 // m_logging = TRUE;
 // m_outputFile.Open(outputFileName, CFile::modeCreate | CFile::modeWrite );
}
//---------------------------------------------------------------------------
NMEAParser::~NMEAParser()
{
  //	if(m_logging)
	  //	m_outputFile.Close();
}
//---------------------------------------------------------------------------
int axtoi( const CHAR *hexStg )
{
  int n = 0;         // position in string
  int m = 0;         // position in digit[] to shift
  int count;         // loop index
  int intValue = 0;  // integer value of hex string
  int digit[5];      // hold values to convert
  while (n < 4)
  {
	 if (hexStg[n]=='\0')
        break;
     if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) //if 0 to 9
        digit[n] = hexStg[n] & 0x0f;            //convert to int
     else if (hexStg[n] >='a' && hexStg[n] <= 'f') //if a to f
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else if (hexStg[n] >='A' && hexStg[n] <= 'F') //if A to F
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else break;
	n++;
  }
  count = n;
  m = n - 1;
  n = 0;
  while(n < count)
  {
     intValue = intValue | (digit[n] << (m << 2));
     m--;   // adjust the position to set
     n++;   // next digit to process
  }
  return (intValue);
}
//---------------------------------------------------------------------------
void NMEAParser::Parse(const CHAR *buf, const UINT bufSize)
{
  //	m_outputFile.Write(buf, bufSize);
	for( UINT i = 0; i < bufSize; i++ )
		ParseRecursive(buf[i]);
}
//---------------------------------------------------------------------------
void NMEAParser::ParseRecursive(const CHAR ch)
{	
  enum NMEAParserState { SearchForSOS = 1,
						 RetrieveAddressField,
                         ReceiveSentenceData,
                         GetFirstChecksumCharacter,
                         GetSecondChecksumCharacter,
                         WaitForST,
                         ValidSentence };

  static const UINT ADDRESS_FIELD_MAX_LENGTH = 10;
  static const UINT NMEA_SEQUENCE_MAX_LENGTH = 81;

  static NMEAParserState m_State = SearchForSOS;
  static UINT m_CalcChecksum;
  static CHAR m_Checksum[3];
  static CHAR m_NMEASequence[NMEA_SEQUENCE_MAX_LENGTH];
  static UINT m_NMEASequenceIndex;
  static CHAR m_AddressField[ADDRESS_FIELD_MAX_LENGTH];
  static UINT m_AddressFieldIndex;

  switch( m_State )
  {
    case SearchForSOS:
    {
	  if( ch == '$' )
	  {
		m_AddressFieldIndex = 0;
	    m_NMEASequenceIndex = 0;
	    m_CalcChecksum = 0;
	    m_State = RetrieveAddressField;
      }
	  break;
	}

    case RetrieveAddressField:
    {
      if( m_NMEASequenceIndex == NMEA_SEQUENCE_MAX_LENGTH - 1 )
        m_State = SearchForSOS;
	  else
      {
        m_NMEASequence[m_NMEASequenceIndex++] = ch;
        m_CalcChecksum ^= ch;
        if( ch == ',' )
 	    {
		  m_AddressField[m_AddressFieldIndex] = '\0';
		  m_State = ReceiveSentenceData;
	    }
	    else if( m_AddressFieldIndex == ADDRESS_FIELD_MAX_LENGTH - 1 ||
	             !isalpha(ch) || islower(ch) )
	      m_State = SearchForSOS;
		else
		  m_AddressField[m_AddressFieldIndex++] = ch;
	  }
	  break;
	}

    case ReceiveSentenceData:
    {
      if( m_NMEASequenceIndex == NMEA_SEQUENCE_MAX_LENGTH - 1 )
        m_State = SearchForSOS;
      else
      {
		m_NMEASequence[m_NMEASequenceIndex++] = ch;
  	    if( ch == '*' )
 	      m_State = GetFirstChecksumCharacter;
 	    else if( ch == 10 )
 	      m_State = WaitForST;
 	    else if( ch == 13 )
        {
		  m_NMEASequence[m_NMEASequenceIndex++] = ch;
          m_NMEASequence[m_NMEASequenceIndex] = '\0';
          ParseNMEASentence( m_AddressField, m_NMEASequence, m_NMEASequenceIndex );
  	      m_State = SearchForSOS;
        }
		else
 	      m_CalcChecksum ^= ch;
	  }
	  break;
    }

    case GetFirstChecksumCharacter:
    {
      if( m_NMEASequenceIndex == NMEA_SEQUENCE_MAX_LENGTH - 1 ||
          ( !isdigit(ch) && ( ch < 'A' || ch > 'F' ) ) )
        m_State = SearchForSOS;
      else
	  {
        m_NMEASequence[m_NMEASequenceIndex++] = ch;
		m_Checksum[0] = ch;
		m_State = GetSecondChecksumCharacter;
	  }
	  break;
	}

    case GetSecondChecksumCharacter:
    {
      if( m_NMEASequenceIndex == NMEA_SEQUENCE_MAX_LENGTH - 1 ||
          ( !isdigit(ch) && ( ch < 'A' || ch > 'F' ) ) )
		m_State = SearchForSOS;
      else
      {
        m_NMEASequence[m_NMEASequenceIndex++] = ch;
		m_Checksum[1] = ch;
		m_Checksum[2] = '\0';
        UINT iChecksum = axtoi( m_Checksum );
        if( iChecksum == m_CalcChecksum )
          m_State = WaitForST;
        else
          m_State = SearchForSOS;
	  }
	  break;
	}

    case WaitForST:
    {
      if( m_NMEASequenceIndex == NMEA_SEQUENCE_MAX_LENGTH - 1 ||
          (ch != 10 && ch != 13) )
        m_State = SearchForSOS;
      else if(ch == 13)
      {
        m_NMEASequence[m_NMEASequenceIndex++] = ch;
        m_NMEASequence[m_NMEASequenceIndex] = '\0';
		ParseNMEASentence( m_AddressField, m_NMEASequence, m_NMEASequenceIndex );
  	    m_State = SearchForSOS;
      }
	  break;
	}
  }

}
//---------------------------------------------------------------------------
void NMEAParser::ParseNMEASentence(const CHAR *addressField,
								   const CHAR *buf, const UINT bufSize)
{
	if( strcmp(addressField, "GPGGA") == NULL )
	{
		ProcessGPGGA(buf, bufSize);
	}
	else if( strcmp(addressField, "GPGSA") == NULL )
	{
		ProcessGPGSA(buf, bufSize);
	}
	else if( strcmp(addressField, "GPGSV") == NULL )
	{
		ProcessGPGSV(buf, bufSize);
	}
	else if( strcmp(addressField, "GPRMB") == NULL )
	{
		ProcessGPRMB(buf, bufSize);
	}
	else if( strcmp(addressField, "GPRMC") == NULL )
	{
		ProcessGPRMC(buf, bufSize);
	}
	else if( strcmp(addressField, "GPZDA") == NULL )
	{
		ProcessGPZDA(buf, bufSize);
	}

	m_GPSInfo.m_dwCommandCount++;
}
//---------------------------------------------------------------------------
GPSInfo& NMEAParser::GetActualGPSInfo()
{
  return m_GPSInfo;
}
//---------------------------------------------------------------------------
/*
GPGGA Sentence format

$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M, ,*47
   |   |	  |			 |			 | |  |	  |		  |      | | 
   |   |	  |			 |			 | |  |	  |		  |		 | checksum data
   |   |	  |			 |			 | |  |	  |		  |		 |
   |   |	  |			 |			 | |  |	  |		  |		 empty field
   |   |	  |			 |			 | |  |	  |		  |
   |   |	  |			 |			 | |  |	  |		  46.9,M Height of geoid (m) above WGS84 ellipsoid
   |   |	  |			 |			 | |  |	  |
   |   |	  |			 |			 | |  |	  545.4,M Altitude (m) above mean sea level
   |   |	  |			 |			 | |  |
   |   |	  |			 |			 | |  0.9 Horizontal dilution of position (HDOP)
   |   |	  |			 |			 | |
   |   |	  |			 |			 | 08 Number of satellites being tracked
   |   |	  |			 |			 |
   |   |	  |			 |			 1 Fix quality:	0 = invalid
   |   |	  |			 |							1 = GPS fix (SPS)
   |   |	  |			 |							2 = DGPS fix
   |   |	  |			 |							3 = PPS fix
   |   |	  |			 |							4 = Real Time Kinematic
   |   |	  |			 |							5 = Float RTK
   |   |	  |			 |							6 = estimated (dead reckoning) (2.3 feature)
   |   |	  |			 |							7 = Manual input mode
   |   |	  |			 |							8 = Simulation mode
   |   |	  |			 |
   |   |	  |			 01131.000,E Longitude 11 deg 31.000' E
   |   |	  |
   |   |	  4807.038,N Latitude 48 deg 07.038' N	
   |   |
   |   123519 Fix taken at 12:35:19 UTC
   |
   GGA Global Positioning System Fix Data

*/
void NMEAParser::ProcessGPGGA(const CHAR *buf, const UINT bufSize)
{
	// To disable handling this sentence uncomment the next line
	// return;


	CHAR auxBuf[10];
	const CHAR *p1 = buf, *p2;

	// GGA
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if(bufSize < 6)
		return;
	strncpy(auxBuf, buf, 5);
	auxBuf[5] = '\0';
	if(strcmp(auxBuf, "GPGGA") != 0 || buf[5] != ',')
		return;
	p1 += 6;

	// Time
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	UINT hour, min, sec;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	hour = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	min = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	sec = atoi(auxBuf);
	p1 = p2 + 1;

	// Latitude
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	if((p2 = strchr(auxBuf, '.')) == NULL)
		return;
	if(p2-auxBuf < 2)
		return;
	DOUBLE latitude = atof(p2 - 2) / 60.0;
	auxBuf[p2 - 2 - auxBuf] = '\0';
	latitude += atof(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 - p1 != 1)
		return;
	if(*p1 == 'S')
		latitude = -latitude;
	else if(*p1 != 'N')
		return;
	p1 = p2 + 1;

    // Longitude
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	if((p2 = strchr(auxBuf, '.')) == NULL)
		return;
	DOUBLE longitude = atof(p2 - 2) / 60.0;
	auxBuf[p2 - 2 - auxBuf] = '\0';
	longitude += atof(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 - p1 != 1)
		return;
	if(*p1 == 'W')
		longitude = -longitude;
	else if(*p1 != 'E')
		return;
	p1 = p2 + 1;

	// GPS quality
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	UINT quality = atoi(auxBuf);

	// Satellites in use
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	UINT satelitesInUse = atoi(auxBuf);
	
	// HDOP
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE hdop = atoi(auxBuf);
	
	// Altitude
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE altitude = atoi(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 - p1 != 1)
		return;
	if(*p1 != 'M')
		return;
	p1 = p2 + 1;

	// Height of geoid
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE heightGeoid = atoi(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
//	if(p2 - p1 != 1)
//		return;
//	if(*p1 != 'M')
//		return;
	p1 = p2 + 1;

	// Empty field
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	p1 = p2 + 1;

	// Last Empty field
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) != NULL)
		return;
	if((p2 = strchr(p1, '*')) == NULL)
		return;

	// Set the values of m_GPSInfo
	m_GPSInfo.m_btGGAHour = hour;
	m_GPSInfo.m_btGGAMinute = min;
	m_GPSInfo.m_btGGASecond = sec;
	m_GPSInfo.m_dGGALatitude = latitude;
	m_GPSInfo.m_dGGALongitude = longitude;
	m_GPSInfo.m_btGGAGPSQuality = quality;
	m_GPSInfo.m_btGGANumOfSatsInUse = satelitesInUse;
	m_GPSInfo.m_dGGAHDOP = hdop;         //
	m_GPSInfo.m_dGGAAltitude = altitude;
	m_GPSInfo.m_dGGAHGEOID = heightGeoid;  //


	//
	// Durive vertical speed (bonus)
	//
	int nSeconds = (int)m_GPSInfo.m_btGGAMinute * 60 + (int)m_GPSInfo.m_btGGASecond;
	if(nSeconds > m_GPSInfo.m_nGGAOldVSpeedSeconds)
	{
		double dDiff = (double)(m_GPSInfo.m_nGGAOldVSpeedSeconds-nSeconds);
		double dVal = dDiff/60.0;
		if(dVal != 0.0)
		{
			m_GPSInfo.m_dGGAVertSpeed = (m_GPSInfo.m_dGGAOldVSpeedAlt - m_GPSInfo.m_dGGAAltitude) / dVal;
		}
	}
	m_GPSInfo.m_dGGAOldVSpeedAlt = m_GPSInfo.m_dGGAAltitude;
	m_GPSInfo.m_nGGAOldVSpeedSeconds = nSeconds;

	// add counter
	m_GPSInfo.m_dwGGACount++;
}
//---------------------------------------------------------------------------
/*
$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39

Where:
     GSA      Satellite status
     A        Auto selection of 2D or 3D fix (M = manual)
     3        3D fix - values include: 1 = no fix
                                       2 = 2D fix
									   3 = 3D fix
     04,05... PRNs of satellites used for fix (space for 12)
     2.5      PDOP (dilution of precision)
     1.3      Horizontal dilution of precision (HDOP)
     2.1      Vertical dilution of precision (VDOP)
	 *39      the checksum data, always begins with *
*/
void NMEAParser::ProcessGPGSA(const CHAR *buf, const UINT bufSize)
{

	CHAR auxBuf[10];
	const CHAR *p1 = buf, *p2;

	//
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if(bufSize < 6)
		return;
	strncpy(auxBuf, buf, 5);
	auxBuf[5] = '\0';
	if(strcmp(auxBuf, "GPGSA") != 0 || buf[5] != ',')
		return;
	p1 += 6;

	// Mode
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	BYTE mode =   auxBuf[0];


	// Fix Mode
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	BYTE fix = atoi(auxBuf);

	// Active satellites
	for(int i = 0; i < 12; i++)
	{
	  if((UINT)(p1 - buf) >= bufSize)
		return;
	  if((p2 = strchr(p1, ',')) == NULL)
		return;
			strncpy(auxBuf, p1, p2 - p1);
			auxBuf[p2 - p1] = '\0';
			if (auxBuf[0] == '\0') {
			  m_GPSInfo.m_wGSASatsInSolution[i] = 0;
			}
			else {
			  m_GPSInfo.m_wGSASatsInSolution[i] = atoi(auxBuf);  // directly on var this time
			}
			p1 = p2 + 1;
	}

	// PDOP
	  if((UINT)(p1 - buf) >= bufSize)
		return;
	  if((p2 = strchr(p1, ',')) == NULL)
		return;
			strncpy(auxBuf, p1, p2 - p1);
			auxBuf[p2 - p1] = '\0';
			m_GPSInfo.m_dGSAPDOP = atof(auxBuf);              // directly on var this time
			p1 = p2 + 1;

	// HDOP
	  if((UINT)(p1 - buf) >= bufSize)
		return;
	  if((p2 = strchr(p1, ',')) == NULL)
		return;
			strncpy(auxBuf, p1, p2 - p1);
			auxBuf[p2 - p1] = '\0';
			m_GPSInfo.m_dGSAHDOP = atof(auxBuf);              // directly on var this time
			p1 = p2 + 1;

	// VDOP
	  if((UINT)(p1 - buf) >= bufSize)
		return;
	  if((p2 = strchr(p1, '*')) == NULL)
		return;
			strncpy(auxBuf, p1, p2 - p1);
			auxBuf[p2 - p1] = '\0';
			m_GPSInfo.m_dGSAVDOP = atof(auxBuf);              // directly on var this time
			p1 = p2 + 1;


		m_GPSInfo.m_btGSAMode = mode;
		m_GPSInfo.m_btGSAFixMode = fix;

  m_GPSInfo.m_dwGSACount++;
}
//---------------------------------------------------------------------------
/*
  $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75

Where:
      GSV          Satellites in view
      2            Number of sentences for full data
      1            sentence 1 of 2
      08           Number of satellites in view

      01           Satellite PRN number
      40           Elevation, degrees
      083          Azimuth, degrees
      46           SNR - higher is better
           for up to 4 satellites per sentence
	  *75          the checksum data, always begins with *

*/
//---------------------------------------------------------------------------
void NMEAParser::ProcessGPGSV(const CHAR *buf, const UINT bufSize)
{
	INT nTotalNumOfMsg, nMsgNum;
	CHAR auxBuf[10];
	const CHAR *p1 = buf, *p2;

	//
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if(bufSize < 6)
		return;
	strncpy(auxBuf, buf, 5);
	auxBuf[5] = '\0';
	if(strcmp(auxBuf, "GPGSV") != 0 || buf[5] != ',')
		return;
	p1 += 6;

	//
	// nr of setences
	//
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';

	nTotalNumOfMsg =   atoi(auxBuf);

	if(nTotalNumOfMsg > 9 || nTotalNumOfMsg < 0)
		return;
	if(nTotalNumOfMsg < 1 || nTotalNumOfMsg*4 >= NP_MAX_CHAN)
		return;
	 p1 = p2 + 1;

	//
	// message number
	//
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';

	nMsgNum = atoi(auxBuf);

	if(nMsgNum > 9 || nMsgNum < 0)       // make sure is valid
		return;
	p1 = p2 + 1;


	//
	// Total satellites in view
	//

	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';

	m_GPSInfo.m_wGSVTotalNumSatsInView =  atoi(auxBuf);
	p1 = p2 + 1;

	//
	// Satelite data
	//


 for(int i = 0; i < 4; i++)
	{
		// Satellite ID
		if((UINT)(p1 - buf) >= bufSize)
			return;
		if((p2 = strchr(p1, ',')) == NULL)
			return;
		strncpy(auxBuf, p1, p2 - p1);
		auxBuf[p2 - p1] = '\0';
		p1 = p2 + 1;

		m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_wPRN = atoi(auxBuf);

		// Elevation
		if((UINT)(p1 - buf) >= bufSize)
			return;
		if((p2 = strchr(p1, ',')) == NULL)
			return;
		strncpy(auxBuf, p1, p2 - p1);
		auxBuf[p2 - p1] = '\0';
		p1 = p2 + 1;

		m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_wElevation =  atoi(auxBuf);

		// Azimuth
		if((UINT)(p1 - buf) >= bufSize)
			return;
		if((p2 = strchr(p1, ',')) == NULL)
			return;
		strncpy(auxBuf, p1, p2 - p1);
		auxBuf[p2 - p1] = '\0';
		p1 = p2 + 1;

		m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_wAzimuth  =  atoi(auxBuf);

		// SNR
		if((UINT)(p1 - buf) >= bufSize)
			return;

		if (i==3) {
			if((p2 = strchr(p1, '*')) == NULL)
			 return;

		}
		else{
			if((p2 = strchr(p1, ',')) == NULL)
			 return;
		}

		strncpy(auxBuf, p1, p2 - p1);
		auxBuf[p2 - p1] = '\0';
		p1 = p2 + 1;

		m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_wSignalQuality =  atoi(auxBuf);

		// Update "used in solution" (m_bUsedInSolution) flag.
		m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_bUsedInSolution = IsSatUsedInSolution(m_GPSInfo.m_GSVSatInfo[i+(nMsgNum-1)*4].m_wPRN);
	 }

   m_GPSInfo.m_dwGSVCount++;
}
//---------------------------------------------------------------------------
void NMEAParser::ProcessGPRMB(const CHAR *buf, const UINT bufSize)
{
   m_GPSInfo.m_dwRMBCount++;
}
//---------------------------------------------------------------------------
/*
Format

  $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
	 |	 |		| |			 |			 |	   |	 |		|	   |
	 |	 |		| |			 |			 |	   |	 |		|	   *6A Checksum data
	 |	 |		| |			 |			 |	   |	 |		|
	 |	 |		| |			 |			 |	   |	 |		003.1,W Magnetic Variation
	 |	 |		| |			 |			 |	   |	 |
	 |	 |		| |			 |			 |	   |	 230394 Date - 23rd of March 1994
	 |	 |		| |			 |			 |	   |
	 |	 |		| |			 |			 |	   084.4 Track angle in degrees
	 |	 |		| |			 |			 |	   
	 |	 |		| |			 |			 022.4 Speed over the ground in knots
	 |	 |		| |			 |
	 |	 |		| |			 01131.000,E Longitude 11 deg 31.000' E
	 |	 |		| |
	 |	 |		| 4807.038,N Latitude 48 deg 07.038' N
	 |	 |		|
	 |	 |		A Status A=active or V=Void
	 |	 |
	 |	 123519 Fix taken at 12:35:19 UTC
	 |
	 RMC Recommended Minimum sentence C

*/
void NMEAParser::ProcessGPRMC(const CHAR *buf, const UINT bufSize)
{
	CHAR auxBuf[10];
	const CHAR *p1 = buf, *p2;

	//
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if(bufSize < 6)
		return;
	strncpy(auxBuf, buf, 5);
	auxBuf[5] = '\0';
	if(strcmp(auxBuf, "GPRMC") != 0 || buf[5] != ',')
		return;
	p1 += 6;

	// Time
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	UINT hour, min, sec;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	hour = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	min = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	sec = atoi(auxBuf);
	p1 = p2 + 1;

	
	// Status 
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 == p1)
		return;
//	if(*p1 != 'A')
//		return;
	p1 = p2 + 1;

    // Latitude
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	if((p2 = strchr(auxBuf, '.')) == NULL)
		return;
	if(p2-auxBuf < 2)
		return;
	DOUBLE latitude = atof(p2 - 2) / 60.0;
	auxBuf[p2 - 2 - auxBuf] = '\0';
	latitude += atof(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 - p1 != 1)
		return;
	if(*p1 == 'S')
		latitude = -latitude;
	else if(*p1 != 'N')
		return;
	p1 = p2 + 1;

    // Longitude
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	if((p2 = strchr(auxBuf, '.')) == NULL)
		return;
	DOUBLE longitude = atof(p2 - 2) / 60.0;
	auxBuf[p2 - 2 - auxBuf] = '\0';
	longitude += atof(auxBuf);
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	if(p2 - p1 != 1)
		return;
	if(*p1 == 'W')
		longitude = -longitude;
	else if(*p1 != 'E')
		return;
	p1 = p2 + 1;

	// Ground speed
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE groundSpeed = atof(auxBuf);

	// Course over ground (degrees) 
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE courseOverGround = atof(auxBuf);

	// Date
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	UINT day, month, year;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	day = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	month = atoi(auxBuf);
	p1 += 2;
	strncpy(auxBuf, p1, 2);
	auxBuf[2] = '\0';
	year = 2000 + atoi(auxBuf);
	p1 = p2 + 1;

	// Magnetic variation
	if((UINT)(p1 - buf) >= bufSize)
		return;
	if((p2 = strchr(p1, ',')) == NULL)
		return;
	strncpy(auxBuf, p1, p2 - p1);
	auxBuf[p2 - p1] = '\0';
	p1 = p2 + 1;
	DOUBLE magneticVariation = atof(auxBuf);
	if((p2 = strchr(p1, '*')) == NULL)
		return;
	if(p2 - p1 > 1)
		return;
	if(*p1 == 'W')
		latitude = -latitude;
	else if(*p1 != 'E' && *p1 != '*')
		return;

	// Set the values of m_GPSInfo

	m_GPSInfo.m_btRMCHour = hour;					//
	m_GPSInfo.m_btRMCMinute = min;					//
	m_GPSInfo.m_btRMCSecond = sec;					//
 //	BYTE m_btRMCDataValid;			            	// A = Data valid, V = navigation rx warning
	m_GPSInfo.m_dRMCLatitude = latitude;			// current latitude
	m_GPSInfo.m_dRMCLongitude = longitude;			// current longitude
	m_GPSInfo.m_dRMCGroundSpeed = groundSpeed;		// speed over ground, knots
	m_GPSInfo.m_dRMCCourse = courseOverGround;		// course over ground, degrees true
	m_GPSInfo.m_btRMCDay = day;				     	//
	m_GPSInfo.m_btRMCMonth = month;					//
	m_GPSInfo.m_wRMCYear = year;					//
	m_GPSInfo.m_dRMCMagVar = magneticVariation;		// magnitic variation, degrees East(+)/West(-)

	m_GPSInfo.m_dwRMCCount++;
}
//---------------------------------------------------------------------------
/*
  $GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC
  $GPZDA,201530.00,04,07,2002,00,00*60

where:
	hhmmss    HrMinSec(UTC)
        dd,mm,yyy Day,Month,Year
        xx        local zone hours -13..13
        yy        local zone minutes 0..59
        *CC       checksum

*/
void NMEAParser::ProcessGPZDA(const CHAR *buf, const UINT bufSize)
{
        CHAR auxBuf[10];
        const CHAR *p1 = buf, *p2;

        if((UINT)(p1 - buf) >= bufSize)
                return;
        if(bufSize < 6)
                return;
        strncpy(auxBuf, buf, 5);
        auxBuf[5] = '\0';
        if(strcmp(auxBuf, "GPZDA") != 0 || buf[5] != ',')
                return;
        p1 += 6;

        // Time
        if((UINT)(p1 - buf) >= bufSize)
                return;
        if((p2 = strchr(p1, ',')) == NULL)
                return;
        UINT hour, min, sec;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        hour = atoi(auxBuf);
        p1 += 2;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        min = atoi(auxBuf);
        p1 += 2;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        sec = atoi(auxBuf);
        p1 = p2 + 1;


        // Date
        if((UINT)(p1 - buf) >= bufSize)
                return;
        if((p2 = strchr(p1, ',')) == NULL)
                return;
        UINT day, month, year;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        day = atoi(auxBuf);
        p1 += 2;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        month = atoi(auxBuf);
        p1 += 2;
        strncpy(auxBuf, p1, 2);
        auxBuf[2] = '\0';
        year = atoi(auxBuf); // full date on this one
        p1 = p2 + 1;		
		
		// Local Zone Time Hour
        if((UINT)(p1 - buf) >= bufSize)
                return;
        if((p2 = strchr(p1, ',')) == NULL)
                return;
        strncpy(auxBuf, p1, p2 - p1);
        auxBuf[p2 - p1] = '\0';

        UINT LocalTimeHour;
		LocalTimeHour =  atoi(auxBuf);
        p1 = p2 + 1;		
		
		//Local Zone Time Min
	    if((UINT)(p1 - buf) >= bufSize)
                return;
        if((p2 = strchr(p1, '*')) == NULL)
                return;
        strncpy(auxBuf, p1, p2 - p1);
        auxBuf[p2 - p1] = '\0';
		UINT LocalTimeMin;
		
        LocalTimeMin = atof(auxBuf);              // directly on var this time
//        p1 = p2 + 1;
		
   m_GPSInfo.m_dwZDACount++;
}
//---------------------------------------------------------------------------
BOOL NMEAParser::IsSatUsedInSolution(WORD wSatID)
{
	if(wSatID == 0) return FALSE;
	for(int i = 0; i < 12; i++)
	{
		if(wSatID ==  m_GPSInfo.m_wGSASatsInSolution[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}
//---------------------------------------------------------------------------