// SystrayPlayer.h: Declares everything needed to use the SystrayPlayer.cpp module
//
// Thread-Safe: PARTLY
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SYSTRAYPLAYER_H
#define __SYSTRAYPLAYER_H


	//////////////////////////////
	// Consts: Player States	//
	//////////////////////////////
	extern const long SYSTRAYPLAYER_STATE_STOP;
	extern const long SYSTRAYPLAYER_STATE_PAUSED;
	extern const long SYSTRAYPLAYER_STATE_PLAYING;


	//////////////////////////////
	// Class: SystrayPlayer		//
	//////////////////////////////
	class SystrayPlayer
	{
	private:
		// Consts: SystrayPlayer
		static const char CAPTION[], TRAY_CAPTION[];
		static const char WNDCLASS[];
		static const long SYSICON_TIMER_INTERVAL;
		static const int  POSBAR_TIMER_ID, SYSICON_TIMER_ID;
		static const long VFW_E_FILEMISSING;

		// Consts: Window Positioning
		static const long DEF_WINDOW_X, DEF_WINDOW_Y;
		static const long WINDOW_WIDTH, WINDOW_HEIGHT;

		// Consts: Runmodes
		static const long MODE_NORMAL, MODE_RAMDISK;

		// Consts: Registry Access
		static const char REG_WINDOW_X[], REG_WINDOW_Y[];
		static const char REG_VOLUME[], REG_REPEAT[], REG_LISTCOUNT[], REG_RAMDRIVE[];
		static const char REG_LASTPLAYINDEX[];
		static const char REG_LASTPLAYSTATE[];
		static const char REG_RANDOMMODE[];
		static const char REG_PLAYLIST_PREFIX[];

		// Types
		class MusicObj
		{
			public:
				string strName;					// GUI name of file (visible to the user).
				string strFile;					// Full path and filename.
				bool bRandomHasPlayed;			// Indicates file has played in current loop of the random mode.
		};

		// Critical Section Object
		CriticalSection cs;

		// Cache
		string ramdisk;

		long runmode;
		bool ctrlinited;

		HWND pwnd;
		HWND lb;
		HWND cmdspeed;					// Speed button
		HWND cmdclear;					// Clear playlist button
		HWND trkpos, trkvol;
		HWND chkrandom;					// Random playback checkbox
		HWND chkrepeat;					// Repeat playback checkbox
		HWND chkshutdown;				// Shutdown-after-playback checkbox
		HWND txtrepeat;
		HDC pdc;
		MemoryDC pwnd_mem;
		StdFonts sfnt;
		
		long pl_state;					// Current Player State: STATE_STOP, STATE_PAUSED, STATE_PLAYING;
		long pl_curfile;				// Current Music File (active and playing)
		long pl_curloop;				// Counter how often the playlist had been played
		long pl_volume;					// Value containing current playback volume
		long pl_random_mode;			// Flag containing if player is in random mode
		double pl_curspeed;				// Value containing current playback speed

		BitmapButton *cmdadd, *cmdremove, *cmdplay, *cmdpause, *cmdstop, *cmdnextfile, *cmdexit;
		SystrayIcon sysicon;
		RegObj reg;
		ActiveMovie* spl_amovie;
		StructArray <MusicObj> mentry;

		string CDLG_FILTER_MULTIMEDIA_FILES;
		string rd_lastfile;

		// Cache: Internal Security Flags
		bool bShutdownAllowed;

		// Cache: Resource Management
		HICON L_IDI_EXECUTE;
		HICON L_IDI_DISK;
		HICON L_IDI_AMOVIE;
		HICON L_IDI_POWER;
		HICON L_IDI_ADDFILE; 
		HICON L_IDI_REMOVE;
		HICON L_IDI_PLAY; 
		HICON L_IDI_PLAYPAUSE;
		HICON L_IDI_STOP;
		HICON L_IDI_NEXT; 
		HICON L_IDI_RESTORE;

		// Functions: Resource Management
		void LoadResources ();
		void FreeResources ();

		// Functions: Init, RamDisk
		bool InitRamdisk();

		// Functions: Window Visibility
		void Window_Show ();
		void Window_Hide ();
		void Window_Toggle ();

		// Cache: Window Visibility
		bool window_visible;

		// Player Control Functions
		void PlayNextFile ();
		bool DecideNextFileToPlay (long*, bool);
		void ChangePlayerState (long);

		// User Interface
		void AddNewFile (string);
		void Command_Add ();
		void Command_Remove ();

		// Additional Functions
		string MediaTime (double);

		// Playlist Storage
		void SavePlaylist ();

		// Ramdisk Support
		long my_amovie_open (string, HWND = 0, long = 0, long = 0);
		void my_amovie_close (HWND = 0);

	public:
		// Constructor, Destructor
		SystrayPlayer();
		~SystrayPlayer();

		// Functions: External Class Interface for "TerminateVKernel_Allowed" decision.
		bool IsShutdownAllowed ();

		// Functions: Callback
		LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

		// Functions: Exported to NTProtect module.
		long getPlayerState ();
		void Command_Play (bool);
		void Command_Pause ();
		void Command_Stop ();
		void Command_Next ();


	protected:
		static LRESULT CALLBACK _ext_SystrayPlayer_WndProc (HWND, UINT, WPARAM, LPARAM);

		static long _ext_SystrayPlayer_TrayMenuFader (MegaPopup*, MegaPopup*, long, long, long, long*);
	};


#endif
