#ifndef __WIN_LOCAL_H__
#define __WIN_LOCAL_H__

#include <windows.h>
#include "../../renderer/GLIncludes/wglext.h"		// windows OpenGL extensions

// WGL_ARB_extensions_string
extern	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;

// WGL_EXT_swap_interval
extern	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

// WGL_ARB_pixel_format
extern	PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
extern	PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB;
extern	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;

// WGL_ARB_pbuffer
extern	PFNWGLCREATEPBUFFERARBPROC	wglCreatePbufferARB;
extern	PFNWGLGETPBUFFERDCARBPROC	wglGetPbufferDCARB;
extern	PFNWGLRELEASEPBUFFERDCARBPROC	wglReleasePbufferDCARB;
extern	PFNWGLDESTROYPBUFFERARBPROC	wglDestroyPbufferARB;
extern	PFNWGLQUERYPBUFFERARBPROC	wglQueryPbufferARB;

// WGL_ARB_render_texture 
extern	PFNWGLBINDTEXIMAGEARBPROC		wglBindTexImageARB;
extern	PFNWGLRELEASETEXIMAGEARBPROC	wglReleaseTexImageARB;
extern	PFNWGLSETPBUFFERATTRIBARBPROC	wglSetPbufferAttribARB;

#define	MAX_OSPATH			256

#define	WINDOW_STYLE( WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE | WS_THICKFRAME )

void	Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

void	Sys_CreateConsole( void );
void	Sys_DestroyConsole( void );

char	*Sys_ConsoleInput( void );
char	*Sys_GetCurrentUser( void );

void	Win_SetErrorText( const char *text );

cpuid_t	Sys_GetCPUId( void );

int		MapKey( intkey);


// Input subsystem

void	IN_Init( void );
void	IN_Shutdown( void );
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void	IN_DeactivateMouseIfWindowed( void );
void	IN_DeactivateMouse( void );
void	IN_ActivateMouse( void );

void	IN_Frame( void );

int		IN_DIMapKey( int key );

void	DisableTaskKeys( BOOL bDisable, BOOL bBeep, BOOL bTaskMgr );


// window procedure
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

void Conbuf_AppendText( const char *msg );

typedef struct {
	HWND			hWnd;
	HINSTANCE		hInstance;

	bool			activeApp;			// changed with WM_ACTIVATE messages
	bool			mouseReleased;		// when the game has the console down or is doing a long operation
	bool			movingWindow;		// inhibit mouse grab when dragging the window
	bool			mouseGrabbed;		// current state of grab and hide

	OSVERSIONINFOEX	osversion;

	cpuid_t			cpuid;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event (not really needed now that we use async direct input)
	int				sysMsgTime;

	bool			windowClassRegistered;

	WNDPROC			wndproc;

	HDC				hDC;							// handle to device context
	HGLRC			hGLRC;						// handle to GL rendering context
	PIXELFORMATDESCRIPTOR pfd;		
	int				pixelformat;

	HINSTANCE		hinstOpenGL;	// HINSTANCE for the OpenGL library

	int				desktopBitsPixel;
	int				desktopWidth, desktopHeight;

	bool			cdsFullscreen;

	FILE			*log_fp;

	unsigned short	oldHardwareGamma[3][256];
	// desktop gamma is saved here for restoration at exit

	static anCVar	sys_arch;
	static anCVar	sys_cpustring;
	static anCVar	in_mouse;
	static anCVar	win_allowAltTab;
	static anCVar	win_notaskkeys;
	static anCVar	win_username;
	static anCVar	win_xpos;			// archived X coordinate of window position
	static anCVar	win_ypos;			// archived Y coordinate of window position
	static anCVar	win_outputDebugString;
	static anCVar	win_outputEditString;
	static anCVar	win_viewlog;
	static anCVar	win_timerUpdate;
	static anCVar	win_allowMultipleInstances;

	CRITICAL_SECTION criticalSections[MAX_CRITICAL_SECTIONS];
	HANDLE			backgroundDownloadSemaphore;

	HINSTANCE		hInstDI;			// direct input

	LPDIRECTINPUT8			g_pdi;
	LPDIRECTINPUTDEVICE8	g_pMouse;
	LPDIRECTINPUTDEVICE8	g_pKeyboard;

	HANDLE			renderCommandsEvent;
	HANDLE			renderCompletedEvent;
	HANDLE			renderActiveEvent;
	HANDLE			renderThreadHandle;
	unsigned long	renderThreadId;
	void			(*glimpRenderThread)( void );
	void			*smpData;
	int				wglErrors;
	// SMP acceleration vars
} Win32Vars_t;

extern Win32Vars_t	win32;

#endif // !__WIN_LOCAL_H__
