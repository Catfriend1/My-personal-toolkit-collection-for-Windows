// ActiveMovie.h: Declares everything needed to use the ActiveMovie.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __ACTIVEMOVIE_H
#define __ACTIVEMOVIE_H


	//////////////
	// Consts	//
	//////////////
	extern const long VOLUME_SILENT;


	//////////////////////////
	// Active Movie Class	//
	//////////////////////////
	class ActiveMovie
	{
	private:
		// Cache
		StructArray <IGraphBuilder*> amvobj;
	
		StructArray <IMediaControl*> mediactrl;
		StructArray <IMediaEvent*> mediaevent;
		StructArray <IMediaEventEx*> mediaeventex;
		StructArray <IMediaSeeking*> mediaseek;
		StructArray <IMediaPosition*> mediapos;
	
		StructArray <IVideoWindow*> videownd;
		StructArray <IBasicVideo*> basicvideo;
		StructArray <IBasicAudio*> basicaudio;

		CriticalSection cs;
	
		bool running;

	public:
		// Init / Terminate
		ActiveMovie ();
		~ActiveMovie ();
	
		// Instance	Management
		long Open (string, HWND = 0, long = 0, long = 0);
		void Close (HWND = 0);
	
		// User Interface
		void Play ();
		void Pause ();
		void Stop ();
		void Home ();
	
		// Stream Information
		double GetPosition ();
		void SetPosition (double);
	
		double GetRate ();
		void SetRate (double);
	
		long GetVolume ();
		void SetVolume (long);
	
		double GetLength ();
	
		// Event Handling
		void SetCallback (HWND, bool, long, long = 0);
		long GetEvent (long*, LONG_PTR*, LONG_PTR*);
		void FreeEvent (long, long, long);
	
		// VideoWindow Related
		void SetVideoWindowParent (HWND);
		void SetMsgTarget (HWND);
		void SetFullscreen (bool);
		void ShowWindow (bool);
		void SetAutoShow (bool);
		void SetWindowPosition (long, long, long, long);
	
		long GetWndStyle ();
		void SetWndStyle (long);
	};


#endif

