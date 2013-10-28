// MotionCapture.cpp : Motion Capture with Kinect and Direct3D
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <time.h>
#include <sstream>
#include <exception>

#include <d3d9.h>
#include <d3dx9.h>

#include <objbase.h>
#include <NuiApi.h>

#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )


// ウィンドウのサイズ
static const int WINDOW_WIDTH  = 800;
static const int WINDOW_HEIGHT = 600;

// ウィンドウのハンドル
static HWND g_mainWindowHandle = 0;

// Win32APIに関する問題が発生したときに投げる例外
class win32_exception : public std::exception
{
public:
	win32_exception( const char *const msg ) : std::exception( msg ) {}
};

// Kinectに関する問題が発生したときに投げる例外
class kinect_exception : public std::exception
{
public:
	kinect_exception( const char *const msg ) : std::exception( msg ) {}
};

// Direct3Dに関する問題が発生したときに投げる例外
class d3d_exception : public std::exception
{
public:
	d3d_exception( const char *const msg ) : std::exception( msg ) {}
};

/*----- フレームレート計測 -----*/

static clock_t g_fpsClk = clock();
static int g_fps = 0;
static int g_fpsCur = 0;

// フレームレートを計測する．1回描画する毎に1度だけこの関数を呼び出す．1秒ごとに更新し，端数は四捨五入する
int updateFps()
{
	const clock_t current = clock();
	const int diff = current - g_fpsClk;
	if( diff >= CLOCKS_PER_SEC ) {
		g_fps = static_cast<int>( ( static_cast<float>( g_fpsCur ) / ( diff / CLOCKS_PER_SEC ) + 0.5f ) );
		g_fpsCur = 0;
		g_fpsClk = ( current / CLOCKS_PER_SEC ) * CLOCKS_PER_SEC + diff % CLOCKS_PER_SEC;
	}
	g_fpsCur++;
	return g_fps;
}

/* ----- Kinect ----- */

// Kinectのインスタンス
static INuiSensor* g_sensor = nullptr;

// RGB画像のストリームと，取得完了と同期するためのハンドル
static HANDLE g_rgbStream = INVALID_HANDLE_VALUE;
static HANDLE g_rgbHandle = INVALID_HANDLE_VALUE;

// Skeletonの取得完了と同期するためのハンドル
static HANDLE g_skeleHandle = INVALID_HANDLE_VALUE;

// Skeletonのフレーム
static NUI_SKELETON_FRAME g_skeleFrame;

// Kinectのチルトモーターの角度
static float g_sensorTiltAngle = 0.0f;
static float g_sensorTiltAnglePrev[ 10 ] = { 0.0f };

// Kinectのリソースを開放する
void releaseKinect()
{
	if( g_sensor ) {
		g_sensor->NuiSkeletonTrackingDisable();
		g_sensor->NuiShutdown();
	}
	CloseHandle( g_rgbHandle );
	CloseHandle( g_skeleHandle );
}

// Kinectを準備する
void initKinect()
{
	HRESULT hResult;

	// Kinectを初期化する
	hResult = NuiCreateSensorByIndex( 0, &g_sensor );
	if( FAILED( hResult ) ) {
		throw kinect_exception(
			"Error : NuiCreateSensorByIndex\nKinectが準備できていないか，使用中です" );
	}

	hResult = g_sensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_SKELETON );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiInitialize" );
	}

	// Kinectと同期するためのイベントを作成する

	g_skeleHandle = CreateEvent( nullptr, TRUE, FALSE, nullptr );
	if( FAILED( g_skeleHandle ) ) {
		throw win32_exception( "Error : CreateEvent" );
	}

	g_rgbHandle = CreateEvent( nullptr, TRUE, FALSE, nullptr );
	if( FAILED( g_rgbHandle ) ) {
		throw win32_exception( "Error : CreateEvent" );
	}

	// RGB画像を取得するためのストリームを開く
	hResult = g_sensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR,
		NUI_IMAGE_RESOLUTION_640x480, 0, 2, g_rgbHandle, &g_rgbStream );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamOpen" );
	}

	// Skeletonの追跡を有効化する
	hResult = g_sensor->NuiSkeletonTrackingEnable( g_skeleHandle, 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiSkeletonTrackingEnable" );
	}

	// チルトモーターの角度を取得する
	long tiltAngle;
	hResult = g_sensor->NuiCameraElevationGetAngle( &tiltAngle );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiCameraElevationGetAngle" );
	}
	for( int i = 0; i < ARRAYSIZE( g_sensorTiltAnglePrev ); i++ ) {
		g_sensorTiltAnglePrev[ i ] = static_cast<float>( tiltAngle );
	}

}

/* ----- Direct3D ----- */

// Boneの長さ[cm]
static const float BONE_LENGTH[ 20 ] = {
	0.0f,  // Root
	5.1f,  // 尻 -> 背
	28.3f, // 背 -> 首
	21.5f, // 首 -> 頭
	19.8f, // 首 -> 左肩
	24.3f, // 左肩 -> 左肘
	26.5f, // 左肘 -> 左手首
	8.2f,  // 左手首 -> 左手
	19.8f, // （体は左右対称なので同じ値とする）
	24.3f,
	26.5f,
	8.2f,
	10.0f, // 尻 -> 左股
	35.8f, // 左股 -> 左膝
	35.2f, // 左膝 -> 左足首
	11.5f, // 左足首 -> 左足
	10.0f,
	35.8f,
	35.2f,
	11.5f
};

// Boneのルートから地面までの距離[cm]
static const float BONE_ROOT_DISTANCE = 108.4f;

// Boneの表示に必要な計算結果を格納するための構造体
struct SkeleState
{
	// Jointの位置
	D3DXVECTOR3 endJointPos[ NUI_SKELETON_POSITION_COUNT ];

	// ローカル変換行列
	D3DXMATRIX localTrans[ NUI_SKELETON_POSITION_COUNT ];

	// トラッキング状態
	bool boneTracked[ NUI_SKELETON_POSITION_COUNT ];

	// 回転や位置などが有効か否か
	bool isValid;

	// メンバー変数を全て初期化する
	void Reset()
	{
		for( int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++ ) {
			endJointPos[ i ].x = 0.0f;
			endJointPos[ i ].y = 0.0f;
			endJointPos[ i ].z = 0.0f;
			D3DXMatrixIdentity( &localTrans[ i ] );
			boneTracked[ i ] = false;
			isValid = false;
		}
	}
};
static SkeleState g_skeleState[ NUI_SKELETON_COUNT ];

// D3Dオブジェクト
static IDirect3D9 *g_d3d = nullptr;

// D3Dデバイス
static IDirect3DDevice9 *g_d3ddev = nullptr;

// Kinectから取得したRGB画像を表示するためのテクスチャー
static IDirect3DTexture9 *g_kinectRgbTex = nullptr;

// 平面と四角錐を表現する3Dモデルの頂点バッファー
static IDirect3DVertexBuffer9 *g_planeVB = nullptr;
static IDirect3DVertexBuffer9 *g_pyramidVB = nullptr;

// 文字
static ID3DXFont *g_font = nullptr;

// 文字サイズ
static const int DEBUG_FONT_SIZE = 24;

// 座標とテクスチャー座標を持つFVF
struct FvfTex {
	FLOAT x, y, z;
	FLOAT u, v;
};

// 座標と色を持つFVF
struct FvfDiffuse {
	FLOAT x, y, z;
	DWORD color;
};

// Direct3Dを解放する
void releaseD3D()
{
	if( g_d3d )           g_d3d->Release();           g_d3d = nullptr;
	if( g_d3ddev )        g_d3ddev->Release();        g_d3ddev = nullptr;
	if( g_kinectRgbTex )  g_kinectRgbTex->Release();  g_kinectRgbTex = nullptr;
	if( g_planeVB )       g_planeVB->Release();       g_planeVB = nullptr;
	if( g_pyramidVB )     g_pyramidVB->Release();     g_pyramidVB = nullptr;
	if( g_font )          g_font->Release();          g_font = nullptr;
}

// Direct3Dを準備する
void initD3D()
{
	HRESULT hResult;

	// Direct3Dオブジェクトを作成する
	g_d3d = Direct3DCreate9( D3D_SDK_VERSION );
	if( g_d3d == nullptr )
		throw d3d_exception( "Error : Direct3DCreate9" );

	// Direct3Dデバイスを作成する
	D3DPRESENT_PARAMETERS presentParam;
	ZeroMemory( &presentParam, sizeof presentParam );
	presentParam.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParam.EnableAutoDepthStencil = TRUE;
	presentParam.AutoDepthStencilFormat = D3DFMT_D24X8;
	presentParam.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	presentParam.hDeviceWindow = g_mainWindowHandle;
	presentParam.Windowed = TRUE;

	hResult = g_d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		g_mainWindowHandle, D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParam, &g_d3ddev );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3D9#CreateDevice" );
	}

	// RGB画像を表示するためのテクスチャーを作成する
	hResult = g_d3ddev->CreateTexture( 640, 480, 1,
		D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,
		&g_kinectRgbTex, nullptr );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#CreateTexture" );
	}

	// 平面のポリゴンを作成する
	void *lockedMem;
	static const FvfTex planeVertex[] = {
		{ -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
		{  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
		{ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
		{  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
	};
	hResult = g_d3ddev->CreateVertexBuffer( sizeof( planeVertex ), 0,
		D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &g_planeVB, nullptr );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#CreateVertexBuffer" );
	}
	
	hResult = g_planeVB->Lock( 0, 0, &lockedMem, D3DLOCK_DISCARD );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DVertexBuffer9#Lock" );
	}

	memcpy( lockedMem, planeVertex, sizeof( planeVertex ) );

	hResult = g_planeVB->Unlock();
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DVertexBuffer9#Unock" );
	}

	// 四角錐のポリゴンを作成する
	static FvfDiffuse pyramidVertex[] = {
		{ -3.0f,  0.0f, -3.0f, 0xFF33AAAA },
		{  3.0f,  0.0f, -3.0f, 0xFF33AAAA },
		{ -3.0f,  0.0f,  3.0f, 0xFF33AAAA },
		
		{ -3.0f,  0.0f,  3.0f, 0xFF33AAAA },
		{  3.0f,  0.0f, -3.0f, 0xFF33AAAA },
		{  3.0f,  0.0f,  3.0f, 0xFF33AAAA },

		{  3.0f,  0.0f, -3.0f, 0xFFEE33BB },
		{ -3.0f,  0.0f, -3.0f, 0xFFEE33BB },
		{  0.0f,  1.0f,  0.0f, 0xFFEE33BB },
		
		{ -3.0f,  0.0f, -3.0f, 0xFFCC33BB },
		{ -3.0f,  0.0f,  3.0f, 0xFFCC33BB },
		{  0.0f,  1.0f,  0.0f, 0xFFCC33BB },
		
		{ -3.0f,  0.0f,  3.0f, 0xFFAA33BB },
		{  3.0f,  0.0f,  3.0f, 0xFFAA33BB },
		{  0.0f,  1.0f,  0.0f, 0xFFAA33BB },
		
		{  3.0f,  0.0f,  3.0f, 0xFF8833BB },
		{  3.0f,  0.0f, -3.0f, 0xFF8833BB },
		{  0.0f,  1.0f,  0.0f, 0xFF8833BB },
	};

	hResult = g_d3ddev->CreateVertexBuffer( sizeof( pyramidVertex ), 0,
		D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_MANAGED, &g_pyramidVB, nullptr );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#CreateVertexBuffer" );
	}
	
	hResult = g_pyramidVB->Lock( 0, 0, &lockedMem, D3DLOCK_DISCARD );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DVertexBuffer9#Lock" );
	}

	memcpy( lockedMem, pyramidVertex, sizeof( pyramidVertex ) );

	hResult = g_pyramidVB->Unlock();
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DVertexBuffer9#Unock" );
	}

	// 文字描画の準備
	D3DXFONT_DESC fontDesc;
	ZeroMemory( &fontDesc, sizeof( fontDesc ) );
	fontDesc.Width = DEBUG_FONT_SIZE / 2;
	fontDesc.Height = DEBUG_FONT_SIZE;
	fontDesc.Weight = FW_NORMAL;
	fontDesc.MipLevels = 1;
	fontDesc.Quality = DEFAULT_QUALITY;
	fontDesc.CharSet = DEFAULT_CHARSET;
	_tcscpy_s( fontDesc.FaceName, _T( "Meiryo UI" ) );

	hResult = D3DXCreateFontIndirect( g_d3ddev, &fontDesc, &g_font );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : D3DXCreateFontIndirect" );
	}
}

/*----- ここからメインの処理 -----*/

// Kinectから情報を取得する
// 更新されたときtrueを返す
bool capture()
{
	HRESULT hResult;
	DWORD dResult;

	// Kinectからのデータの取得が完了しているかどうか調べる
	const HANDLE events[] = { g_rgbHandle, g_skeleHandle };
	dResult = WaitForMultipleObjects( ARRAYSIZE( events ), events, true, 0 );
	// まだのときは取得しない
	if( dResult == WAIT_TIMEOUT ) {
		return false;
	}
	if( dResult == 0xFFFFFFFF ) {
		throw win32_exception( "Error : MsgWaitForMultipleObjects" );
	}

	// RGB画像の新しいフレームを取得する
	NUI_IMAGE_FRAME rgbFrame;
	INuiFrameTexture* rgbTexture;
	NUI_LOCKED_RECT rgbKinectRect;

	hResult = g_sensor->NuiImageStreamGetNextFrame( g_rgbStream, 0, &rgbFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamGetNextFrame" );
	}

	// コピー元のRGB画像の読み取りを開始する
	rgbTexture = rgbFrame.pFrameTexture;
	hResult = rgbTexture->LockRect( 0, &rgbKinectRect, nullptr, 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : INuiFrameTexture#LockRect" );
	}
	
	// コピー先のテクスチャーの書き込みを開始する
	D3DLOCKED_RECT rgbD3DRect;
	hResult = g_kinectRgbTex->LockRect( 0, &rgbD3DRect, nullptr, D3DLOCK_DISCARD );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DTexture9#LockRect" );
	}

	// 1行ずつコピーを繰り返す
	for( int row = 0; row < ( rgbKinectRect.size / rgbKinectRect.Pitch ); row++ ) {
		// コピー元のポインタ
		const byte *psrc = rgbKinectRect.pBits + row * rgbKinectRect.Pitch;
		// コピー先のポインタ
		byte *pdest = reinterpret_cast< byte* >( rgbD3DRect.pBits ) + row * rgbD3DRect.Pitch;
		memcpy( pdest, psrc, rgbKinectRect.Pitch );
	}

	// テクスチャーとRGB画像の転送を完了する
	hResult = g_kinectRgbTex->UnlockRect( 0 );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : Error : IDirect3DTexture9#UnlockRect" );
	}

	hResult = rgbTexture->UnlockRect( 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : INuiFrameTexture#UnlockRect" );
	}

	// RGB画像のフレームを解放する
	hResult = g_sensor->NuiImageStreamReleaseFrame( g_rgbStream, &rgbFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamReleaseFrame" );
	}

	// Skeletonの新しいフレームを取得する
	hResult = g_sensor->NuiSkeletonGetNextFrame( 0, &g_skeleFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiSkeletonGetNextFrame" );
	}

	// Skeletonのスムージング
	const NUI_TRANSFORM_SMOOTH_PARAMETERS customSmooth =
		{0.5f, 0.5f, 0.5f, 0.05f, 0.04f}; // デフォルト
		//{0.5f, 0.1f, 0.5f, 0.1f, 0.1f}; // より平滑化する
		//{0.0f, 1.0f, 0.0f, 0.001f, 0.001f}; // 平滑化しない
	hResult = g_sensor->NuiTransformSmooth( &g_skeleFrame, &customSmooth );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiTransformSmooth" );
	}

	// チルトモーターの角度を取得する
	long tiltAngle;
	hResult = g_sensor->NuiCameraElevationGetAngle( &tiltAngle );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiCameraElevationGetAngle" );
	}

	// 角度を更新する
	for( int i = ARRAYSIZE( g_sensorTiltAnglePrev ) - 1; i >= 1; i-- ) {
		g_sensorTiltAnglePrev[ i ] = g_sensorTiltAnglePrev[ i - 1 ];
	}
	g_sensorTiltAnglePrev[ 0 ] = static_cast<float>( tiltAngle );

	// 角度の平均値を取る
	float angle = 0.0f;
	for( int i = 0; i < ARRAYSIZE( g_sensorTiltAnglePrev ); i++ ) {
		angle += g_sensorTiltAnglePrev[ i ];
	}

	g_sensorTiltAngle = angle / ARRAYSIZE( g_sensorTiltAnglePrev );

	// イベントを非シグナル状態に戻す
	ResetEvent( g_rgbHandle );
	ResetEvent( g_skeleHandle );

	return true;
}

// Matrix4をD3DXMATRIXに変換する
static void convertMat4ToD3DXMat( const Matrix4 &src, D3DXMATRIX &dest )
{
	dest._11 = src.M11; dest._12 = src.M12; dest._13 = src.M13; dest._14 = src.M14;
	dest._21 = src.M21; dest._22 = src.M22; dest._23 = src.M23; dest._24 = src.M24;
	dest._31 = src.M31; dest._32 = src.M32; dest._33 = src.M33; dest._34 = src.M34;
	dest._41 = src.M41; dest._42 = src.M42; dest._43 = src.M43; dest._44 = src.M44;
}

// 座標変換とトラッキング状態をSkeleStateに格納する
static void calcSkeleState(
	const NUI_SKELETON_DATA *skele,
	NUI_SKELETON_POSITION_INDEX start,
	NUI_SKELETON_POSITION_INDEX end,
	const D3DXVECTOR3 &jointStartPos,
	const Matrix4 &rotate,
	float boneLength,
	SkeleState *state
	)
{
	D3DXMATRIX mat;
	D3DXMATRIX matScale, matRotate, matTrans;
	const D3DXVECTOR3 yVec( 0.0f, 1.0f, 0.0f ); // y軸方向に1だけ伸びているベクトル
	D3DXVECTOR3 ret;

	// 3DモデルをY軸方向に拡大する
	D3DXMatrixScaling( &matScale, 1.0f, boneLength, 1.0f );

	// 回転させる
	convertMat4ToD3DXMat( rotate, matRotate );
	
	// 移動させる
	D3DXMatrixTranslation( &matTrans, jointStartPos.x, jointStartPos.y, jointStartPos.z );

	// 変換行列を作成する
	D3DXMatrixMultiply( &mat, &matScale, &matRotate );
	D3DXMatrixMultiply( &state->localTrans[ end ], &mat, &matTrans );

	// 頂点を座標変換する
	D3DXVec3TransformCoord( &state->endJointPos[ end ], &yVec, &state->localTrans[ end ] );

	// Boneのトラッキング状態を設定する
	if( skele->eSkeletonPositionTrackingState[ start ] == NUI_SKELETON_POSITION_TRACKED
		&& skele->eSkeletonPositionTrackingState[ end ] == NUI_SKELETON_POSITION_TRACKED ) {
		state->boneTracked[ end ] = true;
	}
	else {
		state->boneTracked[ end ] = false;
	}
}

// Bone Orientationから表示に必要な情報を求めてSkeleStateに格納する
static void setSkeleStateFromOrient(
	const NUI_SKELETON_DATA *skele,
	const NUI_SKELETON_BONE_ORIENTATION *orient,
	SkeleState *skeleState )
{
	NUI_SKELETON_POSITION_INDEX jointStart, jointEnd;

	jointStart = NUI_SKELETON_POSITION_HIP_CENTER;
	jointEnd   = NUI_SKELETON_POSITION_HIP_CENTER;

	// Rootは表示できるボーンがないので、ローカル変換行列の計算は省略

	// トラッキング状態を取得する
	skeleState->boneTracked[ jointEnd ] =
		skele->eSkeletonPositionTrackingState[ jointEnd ] == NUI_SKELETON_POSITION_TRACKED;

	// Root以外のBoneに対してcalcSkeleState()を呼び出す
	for( int i = 1; i < NUI_SKELETON_POSITION_COUNT; i++ )
	{
		jointStart = orient[ i ].startJoint;
		jointEnd   = orient[ i ].endJoint;

		calcSkeleState( skele, jointStart, jointEnd, skeleState->endJointPos[ jointStart ],
			orient[ jointEnd ].absoluteRotation.rotationMatrix, BONE_LENGTH[ jointEnd ], skeleState );
	}
}

// Kinectからデータを取得した後、回転行列の生成などの計算を事前に処理しておく
void preprocess()
{
	HRESULT hResult;
	for( int i = 0; i < NUI_SKELETON_COUNT; i++ ) {
		NUI_SKELETON_DATA *skele = &g_skeleFrame.SkeletonData[ i ];
		g_skeleState[ i ].Reset();

		// 追跡可能な状態にあれば
		if( skele->eTrackingState == NUI_SKELETON_TRACKED ) {
			// 回転を求め、必要な計算を行う
			NUI_SKELETON_BONE_ORIENTATION orient[ NUI_SKELETON_POSITION_COUNT ];
			hResult = NuiSkeletonCalculateBoneOrientations( skele, orient );

			// 回転が計算できたとき
			if( hResult == S_OK ) {
				setSkeleStateFromOrient( skele, orient, &g_skeleState[ i ] );
				g_skeleState[ i ].isValid = true;
			}
		}
	}
}

// Skeletonを描画する
static void drawSkeleton( NUI_SKELETON_DATA *skele, float floorHeight, float tiltAngle, SkeleState *skeleState )
{
	// チルトモーターの回転角度とSkeletonのルートの位置を元に、ワールド座標系に配置するための変換行列を作る
	D3DXMATRIX matWorld, matTiltRot, matSkeleTrans;

	D3DXMatrixRotationX( &matTiltRot, D3DXToRadian( -1.0f * tiltAngle ) );

	D3DXMatrixTranslation(
		&matSkeleTrans,
		skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].x * 100.0f,
		( skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].y + floorHeight ) * 100.0f - BONE_ROOT_DISTANCE,
		( skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].z ) * 100.0f );

	D3DXMatrixMultiply( &matWorld, &matSkeleTrans, &matTiltRot );

	// 3Dモデルを描画する
	// Rootは描画できないので飛ばす
	for( int i = 1; i < NUI_SKELETON_POSITION_COUNT; i++ ) {
		D3DXMATRIX matLocalWorld;

		// 最終的な変換行列を求める
		D3DXMatrixMultiply( &matLocalWorld, &skeleState->localTrans[ i ], &matWorld );

		// ワールド変換行列を設定する
		g_d3ddev->SetTransform( D3DTS_WORLD, &matLocalWorld );
		
		// Boneがトラッキングできているときは普通に、できていないときはワイヤーフレームで描画する
		if( skeleState->boneTracked[ i ] ) {
			g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
			g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 6 );
		}
		else {
			g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 6 );
		}
	}

	// 描画方法をデフォルトに戻す
	g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

// 描画する
void draw()
{
	HRESULT hResult;

	D3DXVECTOR3 vecEye, vecAt, vecUp;
	D3DXMATRIX matWorld, matView, matProj;
	D3DVIEWPORT9 viewport = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 1.0f };

	RECT textRect = { 0, DEBUG_FONT_SIZE, WINDOW_WIDTH, WINDOW_HEIGHT };


	// デバイスロストへの対応
	hResult = g_d3ddev->TestCooperativeLevel();
	if( hResult == D3DERR_DEVICELOST ) {
		return;
	}
	else if( hResult == D3DERR_DEVICENOTRESET ) {
		releaseD3D();
		initD3D();
	}

	// バッファーをクリアする
	g_d3ddev->Clear( 0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF332211, 1.0f, 0 );
	
	// 描画を開始する
	g_d3ddev->BeginScene();
	
	// 光源を使わない
	g_d3ddev->SetRenderState( D3DRS_LIGHTING, FALSE );

	// ワイヤーフレーム表示のため，カリングを使わない
	g_d3ddev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	// 深度バッファーを使う
	g_d3ddev->SetRenderState( D3DRS_ZENABLE, TRUE );

	// 視点の設定
	vecEye = D3DXVECTOR3( 0.0f, 0.0f,   0.0f );
	vecAt  = D3DXVECTOR3( 0.0f, 0.0f, 400.0f );
	vecUp  = D3DXVECTOR3( 0.0f, 1.0f,   0.0f );
	D3DXMatrixLookAtLH( &matView, &vecEye, &vecAt, &vecUp );
	g_d3ddev->SetTransform( D3DTS_VIEW, &matView );

	// 透視変換
	D3DXMatrixPerspectiveFovLH(
		&matProj, D3DXToRadian( NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV + 5.0f ),
		static_cast<float>( WINDOW_WIDTH ) / WINDOW_HEIGHT, 1.0f, 1000.f );
	g_d3ddev->SetTransform( D3DTS_PROJECTION, &matProj );

	// ビューポートの設定
	g_d3ddev->SetViewport( &viewport );

	/* Skeletonの表示 */

	// 頂点の設定
	g_d3ddev->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE );
	g_d3ddev->SetStreamSource( 0, g_pyramidVB, 0, sizeof( FvfDiffuse ) );
	
	// テクスチャーの設定
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	g_d3ddev->SetTexture( 0, nullptr );

	// 床からの距離を取得する
	float floorHeight;
	if( g_skeleFrame.vFloorClipPlane.y > FLT_EPSILON ) {
		floorHeight = g_skeleFrame.vFloorClipPlane.w;
	}
	// 距離が取得できないときは，既定値（1メートル）を使う
	else {
		floorHeight = 1.0f;
	}

	// Skeletonを描画する
	int numTrackedSkele = 0;
	for( int i = 0; i < NUI_SKELETON_COUNT; i++ ) {
		NUI_SKELETON_DATA skele = g_skeleFrame.SkeletonData[ i ];

		// 追跡可能な状態にあれば
		if( skele.eTrackingState == NUI_SKELETON_TRACKED ) {
			numTrackedSkele++;

			// 回転が計算できるときは
			if( g_skeleState[ i ].isValid ) {
				drawSkeleton( &skele, floorHeight, g_sensorTiltAngle, &g_skeleState[ i ] );
			}
			else {
				g_font->DrawText( nullptr, _T( "回転が計算できないSkeletonがあります" ), -1, &textRect, 0, 0xFFFFFFAA );
				textRect.top += DEBUG_FONT_SIZE;
			}
		}
	}

	// 床からの距離を表示する
	if( g_skeleFrame.vFloorClipPlane.w > FLT_EPSILON ) {
		std::stringstream bufss;
		const Vector4 f = g_skeleFrame.vFloorClipPlane;
		bufss << "Kinectの床からの距離 : " << f.w << "[m]";
		g_font->DrawTextA( nullptr, bufss.str().c_str(), -1, &textRect, 0, 0xFFFFFFFF );
		textRect.top += DEBUG_FONT_SIZE;
	}
	// 床が検出できなかったとき、その旨を表示する
	else {
		g_font->DrawText( nullptr,
			_T( "床が検出できません．カメラの位置を変えてください" ), -1, &textRect, 0, 0xFFFFFFAA );
		textRect.top += DEBUG_FONT_SIZE;
	}

	// 描画するSkeletonが何もなかったとき、その旨を表示する
	if( numTrackedSkele == 0 ) {
		g_font->DrawText( nullptr, _T( "Skeletonが検出できません" ), -1, &textRect, 0, 0xFFFFFFAA );
		textRect.top += DEBUG_FONT_SIZE;
	}
	// あるときは、その数を表示する
	else {
		std::stringstream bufss;
		bufss << "Skeletonの数 : " << numTrackedSkele;
		g_font->DrawTextA( nullptr, bufss.str().c_str(), -1, &textRect, 0, 0xFFAAFFFF );
		textRect.top += DEBUG_FONT_SIZE;
	}

	/* RGB画像の表示 */
	
	// 深度バッファーを使わない
	g_d3ddev->SetRenderState( D3DRS_ZENABLE, FALSE );

	// 頂点の設定
	g_d3ddev->SetFVF( D3DFVF_XYZ | D3DFVF_TEX1 );
	g_d3ddev->SetStreamSource( 0, g_planeVB, 0, sizeof( FvfTex ) );

	// テクスチャーの設定
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	g_d3ddev->SetTexture( 0, g_kinectRgbTex );

	// 座標変換の設定（変換を行わない）
	D3DXMatrixIdentity( &matWorld );
	D3DXMatrixIdentity( &matView );
	D3DXMatrixIdentity( &matProj );
	g_d3ddev->SetTransform( D3DTS_WORLD, &matWorld );
	g_d3ddev->SetTransform( D3DTS_VIEW, &matView );
	g_d3ddev->SetTransform( D3DTS_PROJECTION, &matProj );

	// ビューポートの設定
	viewport.X      = WINDOW_WIDTH * 3 / 4;
	viewport.Width  = WINDOW_WIDTH * 1 / 4;
	viewport.Y      = 0;
	viewport.Height = WINDOW_HEIGHT * 1 / 4;
	g_d3ddev->SetViewport( &viewport );
	
	// 描画
	g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );

	/* フレームレートの表示 */

	// ビューポートの設定
	viewport.X      = 0;
	viewport.Width  = WINDOW_WIDTH;
	viewport.Y      = 0;
	viewport.Height = WINDOW_HEIGHT;
	g_d3ddev->SetViewport( &viewport );

	// フレームレートの取得と描画
	std::stringstream ssFps;
	int fps = updateFps();
	ssFps << "FPS : " << fps;
	textRect.top = 0;
	g_font->DrawTextA( nullptr, ssFps.str().c_str(), -1, &textRect, 0, 0xFFFFFFFF );

	/* 表示おわり */

	// 描画を終了する
	g_d3ddev->EndScene();

	// ディスプレイに転送する
	hResult = g_d3ddev->Present( nullptr, nullptr, nullptr, nullptr );
	if( hResult != D3DERR_DEVICELOST && FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#Present" );
	}
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message ) {
	case WM_KEYDOWN:
		// OpenCVのhighguiの挙動に合わせる
		if( wParam == VK_ESCAPE ) {
			PostMessage( hWnd, WM_DESTROY, 0, 0 );
			return 0;
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		break;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

// ウィンドウを作成する
static HWND setupWindow( int width, int height )
{
	WNDCLASSEX wcex;
	wcex.cbSize         = sizeof( WNDCLASSEX );
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = (HMODULE)GetModuleHandle( 0 );
	wcex.hIcon          = nullptr;
	wcex.hCursor        = LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName   = nullptr;
	wcex.lpszClassName  = _T( "KinectWindowClass" );
	wcex.hIconSm        = nullptr;
	if( !RegisterClassEx( &wcex ) ) {
		throw win32_exception( "Error : RegisterClassEx" );
	}

	RECT rect = { 0, 0, width, height };
	AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
	const int windowWidth  = ( rect.right  - rect.left );
	const int windowHeight = ( rect.bottom - rect.top );

	HWND hWnd = CreateWindow( _T("KinectWindowClass"), _T("# Kinect Skeleton Sample #"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, windowWidth, windowHeight,
		nullptr, nullptr, nullptr, nullptr );
	if( !hWnd ) {
		throw win32_exception( "Error : CreateWindow" );
	}

	return hWnd;
}

// エントリーポイント
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
	MSG msg;
	ZeroMemory( &msg, sizeof msg );

	// Debugでは例外を開発環境が受け取り，Releaseではメッセージを表示して強制的に終了する
#ifdef NDEBUG
	try { 
#endif
	
	g_mainWindowHandle = setupWindow( WINDOW_WIDTH, WINDOW_HEIGHT );
	ShowWindow( g_mainWindowHandle, SW_SHOW );
	UpdateWindow( g_mainWindowHandle );

	// KinectとDirect3Dの準備
	initKinect();
	initD3D();

	while( msg.message != WM_QUIT ) {
		BOOL r = PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE );
		if( r == 0 ) {
			bool updated = capture();
			if( updated ) {
				preprocess();
			}
			draw();
		}
		else {
			DispatchMessage( &msg );
		}
	}

#ifdef NDEBUG
	} catch( std::exception &e ) {
		MessageBoxA( g_mainWindowHandle, e.what(), "Kinect Sample - 問題が発生しました", MB_ICONSTOP ); }
#endif

	// KinectとDirect3Dの終了
	releaseKinect();
	releaseD3D();

	return static_cast<int>( msg.wParam );
}

