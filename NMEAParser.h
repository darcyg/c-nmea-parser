// NMEAParser.h: interface for the NMEAParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NMEAPARSER_H__26C0F55B_19A8_4E71_A1BA_A2EBA169FCEB__INCLUDED_)
#define AFX_NMEAPARSER_H__26C0F55B_19A8_4E71_A1BA_A2EBA169FCEB__INCLUDED_

#include <windows.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define NP_MAX_CHAN				36		// maximum number of channels
#define NP_WAYPOINT_ID_LEN		32		// waypoint max string len

struct CNPSatInfo
{
public:
	WORD	m_wPRN;						//
	WORD	m_wSignalQuality;			//
	BOOL	m_bUsedInSolution;			//
	WORD	m_wAzimuth;					//
	WORD	m_wElevation;				//
};

struct GPSInfo
{
public:

	DWORD m_dwCommandCount;             // number of NMEA commands received (processed)

	//
	// GPGGA Data
	//
	BYTE m_btGGAHour;
	BYTE m_btGGAMinute;
	BYTE m_btGGASecond;
	double m_dGGALatitude;				// < 0 = South, > 0 = North
	double m_dGGALongitude;				// < 0 = West, > 0 = East
	BYTE m_btGGAGPSQuality;				// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	BYTE m_btGGANumOfSatsInUse;			//
	double m_dGGAHDOP;					//
	double m_dGGAAltitude;				// Altitude: mean-sea-level (geoid) meters
	double m_dGGAHGEOID;

	DWORD m_dwGGACount;					//

	int m_nGGAOldVSpeedSeconds;			//
	double m_dGGAOldVSpeedAlt;			//
	double m_dGGAVertSpeed;


	//
	// GPGSA
	//
	BYTE m_btGSAMode;					// M = manual, A = automatic 2D/3D
	BYTE m_btGSAFixMode;				// 1 = fix not available, 2 = 2D, 3 = 3D
	WORD m_wGSASatsInSolution[NP_MAX_CHAN]; // ID of sats in solution
	double m_dGSAPDOP;					//
	double m_dGSAHDOP;					//
	double m_dGSAVDOP;					//
	DWORD m_dwGSACount;					//

	//
	// GPGSV
	//
  //	BYTE m_btGSVTotalNumOfMsg;			    //
	WORD m_wGSVTotalNumSatsInView;	       	//
	CNPSatInfo m_GSVSatInfo[NP_MAX_CHAN];	//
	DWORD m_dwGSVCount;					    //

	//
	// GPRMB
	//
	BYTE m_btRMBDataStatus;				// A = data valid, V = navigation receiver warning
	double m_dRMBCrosstrackError;		// nautical miles
	BYTE m_btRMBDirectionToSteer;		// L/R
	CHAR m_lpszRMBOriginWaypoint[NP_WAYPOINT_ID_LEN]; // Origin Waypoint ID
	CHAR m_lpszRMBDestWaypoint[NP_WAYPOINT_ID_LEN]; // Destination waypoint ID
	double m_dRMBDestLatitude;			// destination waypoint latitude
	double m_dRMBDestLongitude;			// destination waypoint longitude
	double m_dRMBRangeToDest;			// Range to destination nautical mi
	double m_dRMBBearingToDest;			// Bearing to destination, degrees true
	double m_dRMBDestClosingVelocity;	// Destination closing velocity, knots
	BYTE m_btRMBArrivalStatus;			// A = arrival circle entered, V = not entered
	DWORD m_dwRMBCount;					//

	//
	// GPRMC
	//
	BYTE m_btRMCHour;					//
	BYTE m_btRMCMinute;					//
	BYTE m_btRMCSecond;					//
	BYTE m_btRMCDataValid;				// A = Data valid, V = navigation rx warning
	double m_dRMCLatitude;				// current latitude
	double m_dRMCLongitude;				// current longitude
	double m_dRMCGroundSpeed;			// speed over ground, knots
	double m_dRMCCourse;				// course over ground, degrees true
	BYTE m_btRMCDay;					//
	BYTE m_btRMCMonth;					//
	WORD m_wRMCYear;					//
	double m_dRMCMagVar;				// magnitic variation, degrees East(+)/West(-)
	DWORD m_dwRMCCount;					//

	//
	// GPZDA
	//
	BYTE m_btZDAHour;					//
	BYTE m_btZDAMinute;					//
	BYTE m_btZDASecond;					//
	BYTE m_btZDADay;					// 1 - 31
	BYTE m_btZDAMonth;					// 1 - 12
	WORD m_wZDAYear;					//
	BYTE m_btZDALocalZoneHour;			// 0 to +/- 13
	BYTE m_btZDALocalZoneMinute;		// 0 - 59
	DWORD m_dwZDACount;					//
};

class NMEAParser  
{
public:
	NMEAParser();
	NMEAParser(LPCTSTR outputFileName);
	virtual ~NMEAParser();
	void Parse(const CHAR *buf, const UINT bufSize);
	GPSInfo& GetActualGPSInfo();
	GPSInfo m_GPSInfo;


private:
    BOOL IsSatUsedInSolution(WORD wSatID);
	void ParseRecursive(const CHAR ch);
	void ParseNMEASentence(const CHAR *addressField,
						   const CHAR *buf, const UINT bufSize);
	void ProcessGPGGA(const CHAR *buf, const UINT bufSize);
	void ProcessGPGSA(const CHAR *buf, const UINT bufSize);
	void ProcessGPGSV(const CHAR *buf, const UINT bufSize);
	void ProcessGPRMB(const CHAR *buf, const UINT bufSize);
	void ProcessGPRMC(const CHAR *buf, const UINT bufSize);
	void ProcessGPZDA(const CHAR *buf, const UINT bufSize);
};

#endif // !defined(AFX_NMEAPARSER_H__26C0F55B_19A8_4E71_A1BA_A2EBA169FCEB__INCLUDED_)