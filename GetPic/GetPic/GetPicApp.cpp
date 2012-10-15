#include <windows.h>
#include <WinUser.h>
#include <tchar.h>
#include <math.h>
#include <string.h>

HINSTANCE	capInstance;
HWND		hwnd;
HWND		capHwnd;

char bmp_index = 'a';

int width	= 0;
int height	= 0;

RECT captureCoord;
BYTE lClickNum = 0;

bool setTransparentWindow( HWND, int );
HWND createCaptureWindow( int, RECT );

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK captureProc( HWND, UINT, WPARAM, LPARAM );

HBITMAP CopyDCToBitmap(HDC hScrDC, LPRECT lpRect);
//把HBITMAP保存成位图
BOOL SaveBmp(HBITMAP hBitmap, LPCWSTR FileName);

int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	wchar_t appName[] = _T( "Capture your window!" );
	capInstance = hInstance;

	width = GetSystemMetrics( SM_CXVIRTUALSCREEN );
	height = GetSystemMetrics( SM_CYVIRTUALSCREEN );

	WNDCLASSEX wncx;
	ZeroMemory( &wncx, sizeof( wncx ) );
	
	wncx.cbSize = sizeof( WNDCLASSEX );
	wncx.cbClsExtra	= 0;
	wncx.cbWndExtra	= 0;
	wncx.hbrBackground	= (HBRUSH) GetStockObject( TRANSPARENT );
	wncx.hCursor	= LoadCursor( NULL, IDC_ARROW );
	wncx.hIcon		= LoadIcon( NULL, IDI_APPLICATION );
	wncx.hIconSm	= NULL;
	wncx.hInstance	= hInstance;
	wncx.style		= CS_VREDRAW | CS_HREDRAW;
	wncx.lpszClassName	= appName;
	wncx.lpszMenuName	= _T( "Menu" );
	wncx.lpfnWndProc	= (WNDPROC) WndProc;

	if( !RegisterClassEx( &wncx ) )
	{
		MessageBox( NULL, _T( "Register window class failed!" ), _T( "Failed" ), MB_OK );
		return -1;
	}

	hwnd = CreateWindow( appName, _T( "Capture your window" ), WS_POPUP | WS_SYSMENU | WS_SIZEBOX, 0, 0, width, height,
		NULL, NULL, hInstance, NULL );

	if( NULL == hwnd )
	{
		MessageBox( NULL, _T( "Create window failed!" ), _T( "Failed" ), MB_OK );
		return -1;
	}

	ShowWindow( hwnd, nShowCmd );
	UpdateWindow( hwnd );

	MSG msg;
	ZeroMemory( &msg, sizeof( msg ) );

	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}			
		else
		{
			WaitMessage();
		}
	}

	return (int) msg.wParam;
}


LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_CREATE:
		{		
			if( !setTransparentWindow( hwnd, 90 ) )
			{
				MessageBox( NULL, _T( "Set transparent window failed!" ), _T( "Failed" ), MB_OK );
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			if( lClickNum > 1 )
				return 0;

			POINT mousePos;
			GetCursorPos( &mousePos );

			if( lClickNum++ == 0 )
			{
				captureCoord.left = mousePos.x;
				captureCoord.top  = mousePos.y;
			}
			else
			{
				captureCoord.right  = mousePos.x;
				captureCoord.bottom = mousePos.y;

				// 创建截图窗口
				if( !createCaptureWindow( SW_SHOW, captureCoord ) )
				{
					MessageBox( NULL, _T( "Create window failed!" ), _T( "Failed" ), MB_OK );
				}
				else
				{
					MSG msg;
					ZeroMemory( &msg, sizeof( msg ) );

					while( WM_QUIT != msg.message )
					{
						if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
						{
							TranslateMessage( &msg );
							DispatchMessage( &msg );
						}			
						else
						{
							WaitMessage();
						}
					}
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			PostQuitMessage( 0 );
		}
		return 0;

	case WM_DESTROY:
		{
			PostQuitMessage( 0 );
		}
		return 0;

	default:
		return DefWindowProc( hwnd, msg, wParam, lParam );
	}
}

bool setTransparentWindow( HWND hwnd, int trans_degree )
{
	//加入WS_EX_LAYERED扩展属性 
	SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong( hwnd, GWL_EXSTYLE ) ^ 0x80000 ); 

	HINSTANCE hInst = LoadLibrary( _T( "User32.DLL" ) );   
	if( hInst )   
	{   
		typedef BOOL (WINAPI *MYFUNC) ( HWND, COLORREF, BYTE, DWORD );   
		MYFUNC fun = NULL; 

		//取得SetLayeredWindowAttributes函数指针   
		fun = (MYFUNC) GetProcAddress( hInst, "SetLayeredWindowAttributes" ); 

		if( fun )
			fun( hwnd, 0, trans_degree, 2 );   

		FreeLibrary( hInst );   
	} 
	else
		return false;

	return true;
}

HBITMAP CopyDCToBitmap(HDC hScrDC, LPRECT lpRect)
{
	HDC hMemDC; 
	// 屏幕和内存设备描述表
	HBITMAP hBitmap,hOldBitmap; 
	// 位图句柄
	int nX, nY, nX2, nY2; 
	// 选定区域坐标
	int nWidth, nHeight; 
	// 位图宽度和高度
	// 确保选定区域不为空矩形
	if (IsRectEmpty(lpRect))
		return NULL;

	// 获得选定区域坐标
	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;
	nWidth = nX2 - nX;
	nHeight = nY2 - nY;
	//为指定设备描述表创建兼容的内存设备描述表
	hMemDC = CreateCompatibleDC(hScrDC);
	// 创建一个与指定设备描述表兼容的位图
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);
	// 把新位图选到内存设备描述表中
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	// 把屏幕设备描述表拷贝到内存设备描述表中
	StretchBlt(hMemDC,0,0,nWidth,nHeight,hScrDC,nX,nY,nWidth,nHeight,SRCCOPY);
	//BitBlt(hMemDC, 0, 0, nWidth, nHeight,hScrDC, nX, nY, SRCCOPY);
	//得到屏幕位图的句柄 
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);
	//清除 

	DeleteDC(hMemDC);
	DeleteObject(hOldBitmap);
	// 返回位图句柄
	return hBitmap;
}

//把HBITMAP保存成位图
BOOL SaveBmp(HBITMAP hBitmap, LPCWSTR FileName)
{
	HDC hDC;
	//当前分辨率下每象素所占字节数
	int iBits;
	//位图中每象素所占字节数
	WORD wBitCount;
	//定义调色板大小， 位图中像素字节大小 ，位图文件大小 ， 写入文件字节数 
	DWORD dwPaletteSize=0, dwBmBitsSize=0, dwDIBSize=0, dwWritten=0; 
	//位图属性结构 
	BITMAP Bitmap; 
	//位图文件头结构
	BITMAPFILEHEADER bmfHdr; 
	//位图信息头结构 
	BITMAPINFOHEADER bi; 
	//指向位图信息头结构 
	LPBITMAPINFOHEADER lpbi; 
	//定义文件，分配内存句柄，调色板句柄 
	HANDLE fh, hDib, hPal,hOldPal=NULL; 

	//计算位图文件每个像素所占字节数 
	hDC = CreateDC( _T( "DISPLAY" ), NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES); 
	DeleteDC(hDC); 
	if (iBits <= 1) wBitCount = 1; 
	else if (iBits <= 4) wBitCount = 4; 
	else if (iBits <= 8) wBitCount = 8; 
	else wBitCount = 24; 

	GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed = 0;

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存 
	hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER)); 
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib); 
	*lpbi = bi; 
	// 处理调色板 
	hPal = GetStockObject(DEFAULT_PALETTE); 
	if (hPal) 
	{ 
		hDC = ::GetDC(NULL); 
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE); 
		RealizePalette(hDC); 
	}
	// 获取该调色板下新的像素值 
	GetDIBits(hDC, hBitmap, 0, (UINT) Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) 
		+dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS); 

	//恢复调色板 
	if (hOldPal) 
	{ 
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE); 
		RealizePalette(hDC); 
		::ReleaseDC(NULL, hDC); 
	} 
	//创建位图文件 
	fh = CreateFile( FileName, GENERIC_WRITE,0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL ); 

	if (fh == INVALID_HANDLE_VALUE) return FALSE; 

	// 设置位图文件头 
	bmfHdr.bfType = 0x4D42; // "BM" 
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize; 
	bmfHdr.bfSize = dwDIBSize; 
	bmfHdr.bfReserved1 = 0; 
	bmfHdr.bfReserved2 = 0; 
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize; 
	// 写入位图文件头 
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL); 
	// 写入位图文件其余内容 
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL); 
	//清除 
	GlobalUnlock(hDib); 
	GlobalFree(hDib); 
	CloseHandle(fh); 
	return TRUE;
}

HWND createCaptureWindow( int nShowCmd, RECT coord )
{
	wchar_t captureName[] = _T( "Get_it" );

	WNDCLASSEX wncx;
	ZeroMemory( &wncx, sizeof( wncx ) );

	wncx.cbSize = sizeof( WNDCLASSEX );
	wncx.cbClsExtra	= 0;
	wncx.cbWndExtra	= 0;
	wncx.hbrBackground	= (HBRUSH) GetStockObject( TRANSPARENT );
	wncx.hCursor	= LoadCursor( NULL, IDC_WAIT );
	wncx.hIcon		= LoadIcon( NULL, IDI_APPLICATION );
	wncx.hIconSm	= NULL;
	wncx.hInstance	= capInstance;
	wncx.style		= CS_VREDRAW | CS_HREDRAW;
	wncx.lpszClassName	= captureName;
	wncx.lpszMenuName	= _T( "Menu" );
	wncx.lpfnWndProc	= (WNDPROC) captureProc;

	if( !RegisterClassEx( &wncx ) )
	{
		MessageBox( NULL, _T( "Register window class failed!" ), _T( "Failed" ), MB_OK );
		return 0;
	}

	int initX = min( coord.left, coord.right );
	int initY = min( coord.top, coord.bottom );
	int initWidth = abs( coord.right - coord.left );
	int initHeight = abs( coord.bottom - coord.top );

	capHwnd = CreateWindow( captureName, _T( "Capture your window" ), WS_POPUP | WS_SYSMENU | WS_SIZEBOX | WS_CHILD, initX, initY, initWidth, initHeight,
		hwnd, NULL, capInstance, NULL );

	if( NULL == capHwnd )
	{
		MessageBox( NULL, _T( "Create window failed!" ), _T( "Failed" ), MB_OK );
		return 0;
	}

	ShowWindow( capHwnd, nShowCmd );
	UpdateWindow( capHwnd );

	return capHwnd;
}

LRESULT CALLBACK captureProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_CREATE:
		{
			if( !setTransparentWindow( hwnd, 60 ) )
			{
				MessageBox( NULL, _T( "Set transparent window failed!" ), _T( "Failed" ), MB_OK );
			}
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint( hwnd, &ps );
			::EndPaint( hwnd, &ps );
			return 0;
		}

	case WM_KEYDOWN:
		{
			switch( wParam )
			{
			case VK_ESCAPE:
				lClickNum = 0;
				SendMessage( capHwnd, WM_DESTROY, wParam, lParam );
				break;

			default:
				break;
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			wchar_t bmp_file[] = { ' ', '.', 'b', 'm', 'p', '\0' };
			bmp_file[0] = bmp_index++;

			GetWindowRect( capHwnd, &captureCoord );

			if( !SaveBmp( CopyDCToBitmap( GetDC( GetDesktopWindow() ), &captureCoord ), bmp_file ) )
			{
				MessageBox( NULL, _T( "Create bitmap failed!" ), _T( "Failed" ), MB_OK );
			}
			else
			{
				MessageBox( NULL, _T( "Create bitmap successfully!" ), _T( "Succeed" ), MB_OK );
			}
		}
		return 0;

	case WM_NCHITTEST:
		{
			UINT nHitTest;
			nHitTest = ::DefWindowProc(hwnd,msg,wParam,lParam);
			//如果鼠标左键按下, GetAsyncKeyState 的返回值小于0
			if(nHitTest == HTCLIENT && ::GetAsyncKeyState(MK_LBUTTON) < 0)
			{
				nHitTest = HTCAPTION;
			}
			return nHitTest;
		}

	case WM_DESTROY:
		{
			PostQuitMessage( 0 );
		}
		return 0;

	default:
		return DefWindowProc( hwnd, msg, wParam, lParam );
	}
}