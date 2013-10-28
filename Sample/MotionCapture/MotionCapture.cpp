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


// �E�B���h�E�̃T�C�Y
static const int WINDOW_WIDTH  = 800;
static const int WINDOW_HEIGHT = 600;

// �E�B���h�E�̃n���h��
static HWND g_mainWindowHandle = 0;

// Win32API�Ɋւ����肪���������Ƃ��ɓ������O
class win32_exception : public std::exception
{
public:
	win32_exception( const char *const msg ) : std::exception( msg ) {}
};

// Kinect�Ɋւ����肪���������Ƃ��ɓ������O
class kinect_exception : public std::exception
{
public:
	kinect_exception( const char *const msg ) : std::exception( msg ) {}
};

// Direct3D�Ɋւ����肪���������Ƃ��ɓ������O
class d3d_exception : public std::exception
{
public:
	d3d_exception( const char *const msg ) : std::exception( msg ) {}
};

/*----- �t���[�����[�g�v�� -----*/

static clock_t g_fpsClk = clock();
static int g_fps = 0;
static int g_fpsCur = 0;

// �t���[�����[�g���v������D1��`�悷�閈��1�x�������̊֐����Ăяo���D1�b���ƂɍX�V���C�[���͎l�̌ܓ�����
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

// Kinect�̃C���X�^���X
static INuiSensor* g_sensor = nullptr;

// RGB�摜�̃X�g���[���ƁC�擾�����Ɠ������邽�߂̃n���h��
static HANDLE g_rgbStream = INVALID_HANDLE_VALUE;
static HANDLE g_rgbHandle = INVALID_HANDLE_VALUE;

// Skeleton�̎擾�����Ɠ������邽�߂̃n���h��
static HANDLE g_skeleHandle = INVALID_HANDLE_VALUE;

// Skeleton�̃t���[��
static NUI_SKELETON_FRAME g_skeleFrame;

// Kinect�̃`���g���[�^�[�̊p�x
static float g_sensorTiltAngle = 0.0f;
static float g_sensorTiltAnglePrev[ 10 ] = { 0.0f };

// Kinect�̃��\�[�X���J������
void releaseKinect()
{
	if( g_sensor ) {
		g_sensor->NuiSkeletonTrackingDisable();
		g_sensor->NuiShutdown();
	}
	CloseHandle( g_rgbHandle );
	CloseHandle( g_skeleHandle );
}

// Kinect����������
void initKinect()
{
	HRESULT hResult;

	// Kinect������������
	hResult = NuiCreateSensorByIndex( 0, &g_sensor );
	if( FAILED( hResult ) ) {
		throw kinect_exception(
			"Error : NuiCreateSensorByIndex\nKinect�������ł��Ă��Ȃ����C�g�p���ł�" );
	}

	hResult = g_sensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_SKELETON );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiInitialize" );
	}

	// Kinect�Ɠ������邽�߂̃C�x���g���쐬����

	g_skeleHandle = CreateEvent( nullptr, TRUE, FALSE, nullptr );
	if( FAILED( g_skeleHandle ) ) {
		throw win32_exception( "Error : CreateEvent" );
	}

	g_rgbHandle = CreateEvent( nullptr, TRUE, FALSE, nullptr );
	if( FAILED( g_rgbHandle ) ) {
		throw win32_exception( "Error : CreateEvent" );
	}

	// RGB�摜���擾���邽�߂̃X�g���[�����J��
	hResult = g_sensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR,
		NUI_IMAGE_RESOLUTION_640x480, 0, 2, g_rgbHandle, &g_rgbStream );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamOpen" );
	}

	// Skeleton�̒ǐՂ�L��������
	hResult = g_sensor->NuiSkeletonTrackingEnable( g_skeleHandle, 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiSkeletonTrackingEnable" );
	}

	// �`���g���[�^�[�̊p�x���擾����
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

// Bone�̒���[cm]
static const float BONE_LENGTH[ 20 ] = {
	0.0f,  // Root
	5.1f,  // �K -> �w
	28.3f, // �w -> ��
	21.5f, // �� -> ��
	19.8f, // �� -> ����
	24.3f, // ���� -> ���I
	26.5f, // ���I -> �����
	8.2f,  // ����� -> ����
	19.8f, // �i�͍̂��E�Ώ̂Ȃ̂œ����l�Ƃ���j
	24.3f,
	26.5f,
	8.2f,
	10.0f, // �K -> ����
	35.8f, // ���� -> ���G
	35.2f, // ���G -> ������
	11.5f, // ������ -> ����
	10.0f,
	35.8f,
	35.2f,
	11.5f
};

// Bone�̃��[�g����n�ʂ܂ł̋���[cm]
static const float BONE_ROOT_DISTANCE = 108.4f;

// Bone�̕\���ɕK�v�Ȍv�Z���ʂ��i�[���邽�߂̍\����
struct SkeleState
{
	// Joint�̈ʒu
	D3DXVECTOR3 endJointPos[ NUI_SKELETON_POSITION_COUNT ];

	// ���[�J���ϊ��s��
	D3DXMATRIX localTrans[ NUI_SKELETON_POSITION_COUNT ];

	// �g���b�L���O���
	bool boneTracked[ NUI_SKELETON_POSITION_COUNT ];

	// ��]��ʒu�Ȃǂ��L�����ۂ�
	bool isValid;

	// �����o�[�ϐ���S�ď���������
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

// D3D�I�u�W�F�N�g
static IDirect3D9 *g_d3d = nullptr;

// D3D�f�o�C�X
static IDirect3DDevice9 *g_d3ddev = nullptr;

// Kinect����擾����RGB�摜��\�����邽�߂̃e�N�X�`���[
static IDirect3DTexture9 *g_kinectRgbTex = nullptr;

// ���ʂƎl�p����\������3D���f���̒��_�o�b�t�@�[
static IDirect3DVertexBuffer9 *g_planeVB = nullptr;
static IDirect3DVertexBuffer9 *g_pyramidVB = nullptr;

// ����
static ID3DXFont *g_font = nullptr;

// �����T�C�Y
static const int DEBUG_FONT_SIZE = 24;

// ���W�ƃe�N�X�`���[���W������FVF
struct FvfTex {
	FLOAT x, y, z;
	FLOAT u, v;
};

// ���W�ƐF������FVF
struct FvfDiffuse {
	FLOAT x, y, z;
	DWORD color;
};

// Direct3D���������
void releaseD3D()
{
	if( g_d3d )           g_d3d->Release();           g_d3d = nullptr;
	if( g_d3ddev )        g_d3ddev->Release();        g_d3ddev = nullptr;
	if( g_kinectRgbTex )  g_kinectRgbTex->Release();  g_kinectRgbTex = nullptr;
	if( g_planeVB )       g_planeVB->Release();       g_planeVB = nullptr;
	if( g_pyramidVB )     g_pyramidVB->Release();     g_pyramidVB = nullptr;
	if( g_font )          g_font->Release();          g_font = nullptr;
}

// Direct3D����������
void initD3D()
{
	HRESULT hResult;

	// Direct3D�I�u�W�F�N�g���쐬����
	g_d3d = Direct3DCreate9( D3D_SDK_VERSION );
	if( g_d3d == nullptr )
		throw d3d_exception( "Error : Direct3DCreate9" );

	// Direct3D�f�o�C�X���쐬����
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

	// RGB�摜��\�����邽�߂̃e�N�X�`���[���쐬����
	hResult = g_d3ddev->CreateTexture( 640, 480, 1,
		D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,
		&g_kinectRgbTex, nullptr );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#CreateTexture" );
	}

	// ���ʂ̃|���S�����쐬����
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

	// �l�p���̃|���S�����쐬����
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

	// �����`��̏���
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

/*----- �������烁�C���̏��� -----*/

// Kinect��������擾����
// �X�V���ꂽ�Ƃ�true��Ԃ�
bool capture()
{
	HRESULT hResult;
	DWORD dResult;

	// Kinect����̃f�[�^�̎擾���������Ă��邩�ǂ������ׂ�
	const HANDLE events[] = { g_rgbHandle, g_skeleHandle };
	dResult = WaitForMultipleObjects( ARRAYSIZE( events ), events, true, 0 );
	// �܂��̂Ƃ��͎擾���Ȃ�
	if( dResult == WAIT_TIMEOUT ) {
		return false;
	}
	if( dResult == 0xFFFFFFFF ) {
		throw win32_exception( "Error : MsgWaitForMultipleObjects" );
	}

	// RGB�摜�̐V�����t���[�����擾����
	NUI_IMAGE_FRAME rgbFrame;
	INuiFrameTexture* rgbTexture;
	NUI_LOCKED_RECT rgbKinectRect;

	hResult = g_sensor->NuiImageStreamGetNextFrame( g_rgbStream, 0, &rgbFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamGetNextFrame" );
	}

	// �R�s�[����RGB�摜�̓ǂݎ����J�n����
	rgbTexture = rgbFrame.pFrameTexture;
	hResult = rgbTexture->LockRect( 0, &rgbKinectRect, nullptr, 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : INuiFrameTexture#LockRect" );
	}
	
	// �R�s�[��̃e�N�X�`���[�̏������݂��J�n����
	D3DLOCKED_RECT rgbD3DRect;
	hResult = g_kinectRgbTex->LockRect( 0, &rgbD3DRect, nullptr, D3DLOCK_DISCARD );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DTexture9#LockRect" );
	}

	// 1�s���R�s�[���J��Ԃ�
	for( int row = 0; row < ( rgbKinectRect.size / rgbKinectRect.Pitch ); row++ ) {
		// �R�s�[���̃|�C���^
		const byte *psrc = rgbKinectRect.pBits + row * rgbKinectRect.Pitch;
		// �R�s�[��̃|�C���^
		byte *pdest = reinterpret_cast< byte* >( rgbD3DRect.pBits ) + row * rgbD3DRect.Pitch;
		memcpy( pdest, psrc, rgbKinectRect.Pitch );
	}

	// �e�N�X�`���[��RGB�摜�̓]������������
	hResult = g_kinectRgbTex->UnlockRect( 0 );
	if( FAILED( hResult ) ) {
		throw d3d_exception( "Error : Error : IDirect3DTexture9#UnlockRect" );
	}

	hResult = rgbTexture->UnlockRect( 0 );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : INuiFrameTexture#UnlockRect" );
	}

	// RGB�摜�̃t���[�����������
	hResult = g_sensor->NuiImageStreamReleaseFrame( g_rgbStream, &rgbFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiImageStreamReleaseFrame" );
	}

	// Skeleton�̐V�����t���[�����擾����
	hResult = g_sensor->NuiSkeletonGetNextFrame( 0, &g_skeleFrame );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiSkeletonGetNextFrame" );
	}

	// Skeleton�̃X���[�W���O
	const NUI_TRANSFORM_SMOOTH_PARAMETERS customSmooth =
		{0.5f, 0.5f, 0.5f, 0.05f, 0.04f}; // �f�t�H���g
		//{0.5f, 0.1f, 0.5f, 0.1f, 0.1f}; // ��蕽��������
		//{0.0f, 1.0f, 0.0f, 0.001f, 0.001f}; // ���������Ȃ�
	hResult = g_sensor->NuiTransformSmooth( &g_skeleFrame, &customSmooth );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiTransformSmooth" );
	}

	// �`���g���[�^�[�̊p�x���擾����
	long tiltAngle;
	hResult = g_sensor->NuiCameraElevationGetAngle( &tiltAngle );
	if( FAILED( hResult ) ) {
		throw kinect_exception( "Error : NuiCameraElevationGetAngle" );
	}

	// �p�x���X�V����
	for( int i = ARRAYSIZE( g_sensorTiltAnglePrev ) - 1; i >= 1; i-- ) {
		g_sensorTiltAnglePrev[ i ] = g_sensorTiltAnglePrev[ i - 1 ];
	}
	g_sensorTiltAnglePrev[ 0 ] = static_cast<float>( tiltAngle );

	// �p�x�̕��ϒl�����
	float angle = 0.0f;
	for( int i = 0; i < ARRAYSIZE( g_sensorTiltAnglePrev ); i++ ) {
		angle += g_sensorTiltAnglePrev[ i ];
	}

	g_sensorTiltAngle = angle / ARRAYSIZE( g_sensorTiltAnglePrev );

	// �C�x���g���V�O�i����Ԃɖ߂�
	ResetEvent( g_rgbHandle );
	ResetEvent( g_skeleHandle );

	return true;
}

// Matrix4��D3DXMATRIX�ɕϊ�����
static void convertMat4ToD3DXMat( const Matrix4 &src, D3DXMATRIX &dest )
{
	dest._11 = src.M11; dest._12 = src.M12; dest._13 = src.M13; dest._14 = src.M14;
	dest._21 = src.M21; dest._22 = src.M22; dest._23 = src.M23; dest._24 = src.M24;
	dest._31 = src.M31; dest._32 = src.M32; dest._33 = src.M33; dest._34 = src.M34;
	dest._41 = src.M41; dest._42 = src.M42; dest._43 = src.M43; dest._44 = src.M44;
}

// ���W�ϊ��ƃg���b�L���O��Ԃ�SkeleState�Ɋi�[����
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
	const D3DXVECTOR3 yVec( 0.0f, 1.0f, 0.0f ); // y��������1�����L�тĂ���x�N�g��
	D3DXVECTOR3 ret;

	// 3D���f����Y�������Ɋg�傷��
	D3DXMatrixScaling( &matScale, 1.0f, boneLength, 1.0f );

	// ��]������
	convertMat4ToD3DXMat( rotate, matRotate );
	
	// �ړ�������
	D3DXMatrixTranslation( &matTrans, jointStartPos.x, jointStartPos.y, jointStartPos.z );

	// �ϊ��s����쐬����
	D3DXMatrixMultiply( &mat, &matScale, &matRotate );
	D3DXMatrixMultiply( &state->localTrans[ end ], &mat, &matTrans );

	// ���_�����W�ϊ�����
	D3DXVec3TransformCoord( &state->endJointPos[ end ], &yVec, &state->localTrans[ end ] );

	// Bone�̃g���b�L���O��Ԃ�ݒ肷��
	if( skele->eSkeletonPositionTrackingState[ start ] == NUI_SKELETON_POSITION_TRACKED
		&& skele->eSkeletonPositionTrackingState[ end ] == NUI_SKELETON_POSITION_TRACKED ) {
		state->boneTracked[ end ] = true;
	}
	else {
		state->boneTracked[ end ] = false;
	}
}

// Bone Orientation����\���ɕK�v�ȏ������߂�SkeleState�Ɋi�[����
static void setSkeleStateFromOrient(
	const NUI_SKELETON_DATA *skele,
	const NUI_SKELETON_BONE_ORIENTATION *orient,
	SkeleState *skeleState )
{
	NUI_SKELETON_POSITION_INDEX jointStart, jointEnd;

	jointStart = NUI_SKELETON_POSITION_HIP_CENTER;
	jointEnd   = NUI_SKELETON_POSITION_HIP_CENTER;

	// Root�͕\���ł���{�[�����Ȃ��̂ŁA���[�J���ϊ��s��̌v�Z�͏ȗ�

	// �g���b�L���O��Ԃ��擾����
	skeleState->boneTracked[ jointEnd ] =
		skele->eSkeletonPositionTrackingState[ jointEnd ] == NUI_SKELETON_POSITION_TRACKED;

	// Root�ȊO��Bone�ɑ΂���calcSkeleState()���Ăяo��
	for( int i = 1; i < NUI_SKELETON_POSITION_COUNT; i++ )
	{
		jointStart = orient[ i ].startJoint;
		jointEnd   = orient[ i ].endJoint;

		calcSkeleState( skele, jointStart, jointEnd, skeleState->endJointPos[ jointStart ],
			orient[ jointEnd ].absoluteRotation.rotationMatrix, BONE_LENGTH[ jointEnd ], skeleState );
	}
}

// Kinect����f�[�^���擾������A��]�s��̐����Ȃǂ̌v�Z�����O�ɏ������Ă���
void preprocess()
{
	HRESULT hResult;
	for( int i = 0; i < NUI_SKELETON_COUNT; i++ ) {
		NUI_SKELETON_DATA *skele = &g_skeleFrame.SkeletonData[ i ];
		g_skeleState[ i ].Reset();

		// �ǐՉ\�ȏ�Ԃɂ����
		if( skele->eTrackingState == NUI_SKELETON_TRACKED ) {
			// ��]�����߁A�K�v�Ȍv�Z���s��
			NUI_SKELETON_BONE_ORIENTATION orient[ NUI_SKELETON_POSITION_COUNT ];
			hResult = NuiSkeletonCalculateBoneOrientations( skele, orient );

			// ��]���v�Z�ł����Ƃ�
			if( hResult == S_OK ) {
				setSkeleStateFromOrient( skele, orient, &g_skeleState[ i ] );
				g_skeleState[ i ].isValid = true;
			}
		}
	}
}

// Skeleton��`�悷��
static void drawSkeleton( NUI_SKELETON_DATA *skele, float floorHeight, float tiltAngle, SkeleState *skeleState )
{
	// �`���g���[�^�[�̉�]�p�x��Skeleton�̃��[�g�̈ʒu�����ɁA���[���h���W�n�ɔz�u���邽�߂̕ϊ��s������
	D3DXMATRIX matWorld, matTiltRot, matSkeleTrans;

	D3DXMatrixRotationX( &matTiltRot, D3DXToRadian( -1.0f * tiltAngle ) );

	D3DXMatrixTranslation(
		&matSkeleTrans,
		skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].x * 100.0f,
		( skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].y + floorHeight ) * 100.0f - BONE_ROOT_DISTANCE,
		( skele->SkeletonPositions[ NUI_SKELETON_POSITION_HIP_CENTER ].z ) * 100.0f );

	D3DXMatrixMultiply( &matWorld, &matSkeleTrans, &matTiltRot );

	// 3D���f����`�悷��
	// Root�͕`��ł��Ȃ��̂Ŕ�΂�
	for( int i = 1; i < NUI_SKELETON_POSITION_COUNT; i++ ) {
		D3DXMATRIX matLocalWorld;

		// �ŏI�I�ȕϊ��s������߂�
		D3DXMatrixMultiply( &matLocalWorld, &skeleState->localTrans[ i ], &matWorld );

		// ���[���h�ϊ��s���ݒ肷��
		g_d3ddev->SetTransform( D3DTS_WORLD, &matLocalWorld );
		
		// Bone���g���b�L���O�ł��Ă���Ƃ��͕��ʂɁA�ł��Ă��Ȃ��Ƃ��̓��C���[�t���[���ŕ`�悷��
		if( skeleState->boneTracked[ i ] ) {
			g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
			g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 6 );
		}
		else {
			g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 6 );
		}
	}

	// �`����@���f�t�H���g�ɖ߂�
	g_d3ddev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

// �`�悷��
void draw()
{
	HRESULT hResult;

	D3DXVECTOR3 vecEye, vecAt, vecUp;
	D3DXMATRIX matWorld, matView, matProj;
	D3DVIEWPORT9 viewport = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 1.0f };

	RECT textRect = { 0, DEBUG_FONT_SIZE, WINDOW_WIDTH, WINDOW_HEIGHT };


	// �f�o�C�X���X�g�ւ̑Ή�
	hResult = g_d3ddev->TestCooperativeLevel();
	if( hResult == D3DERR_DEVICELOST ) {
		return;
	}
	else if( hResult == D3DERR_DEVICENOTRESET ) {
		releaseD3D();
		initD3D();
	}

	// �o�b�t�@�[���N���A����
	g_d3ddev->Clear( 0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF332211, 1.0f, 0 );
	
	// �`����J�n����
	g_d3ddev->BeginScene();
	
	// �������g��Ȃ�
	g_d3ddev->SetRenderState( D3DRS_LIGHTING, FALSE );

	// ���C���[�t���[���\���̂��߁C�J�����O���g��Ȃ�
	g_d3ddev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	// �[�x�o�b�t�@�[���g��
	g_d3ddev->SetRenderState( D3DRS_ZENABLE, TRUE );

	// ���_�̐ݒ�
	vecEye = D3DXVECTOR3( 0.0f, 0.0f,   0.0f );
	vecAt  = D3DXVECTOR3( 0.0f, 0.0f, 400.0f );
	vecUp  = D3DXVECTOR3( 0.0f, 1.0f,   0.0f );
	D3DXMatrixLookAtLH( &matView, &vecEye, &vecAt, &vecUp );
	g_d3ddev->SetTransform( D3DTS_VIEW, &matView );

	// �����ϊ�
	D3DXMatrixPerspectiveFovLH(
		&matProj, D3DXToRadian( NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV + 5.0f ),
		static_cast<float>( WINDOW_WIDTH ) / WINDOW_HEIGHT, 1.0f, 1000.f );
	g_d3ddev->SetTransform( D3DTS_PROJECTION, &matProj );

	// �r���[�|�[�g�̐ݒ�
	g_d3ddev->SetViewport( &viewport );

	/* Skeleton�̕\�� */

	// ���_�̐ݒ�
	g_d3ddev->SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE );
	g_d3ddev->SetStreamSource( 0, g_pyramidVB, 0, sizeof( FvfDiffuse ) );
	
	// �e�N�X�`���[�̐ݒ�
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	g_d3ddev->SetTexture( 0, nullptr );

	// ������̋������擾����
	float floorHeight;
	if( g_skeleFrame.vFloorClipPlane.y > FLT_EPSILON ) {
		floorHeight = g_skeleFrame.vFloorClipPlane.w;
	}
	// �������擾�ł��Ȃ��Ƃ��́C����l�i1���[�g���j���g��
	else {
		floorHeight = 1.0f;
	}

	// Skeleton��`�悷��
	int numTrackedSkele = 0;
	for( int i = 0; i < NUI_SKELETON_COUNT; i++ ) {
		NUI_SKELETON_DATA skele = g_skeleFrame.SkeletonData[ i ];

		// �ǐՉ\�ȏ�Ԃɂ����
		if( skele.eTrackingState == NUI_SKELETON_TRACKED ) {
			numTrackedSkele++;

			// ��]���v�Z�ł���Ƃ���
			if( g_skeleState[ i ].isValid ) {
				drawSkeleton( &skele, floorHeight, g_sensorTiltAngle, &g_skeleState[ i ] );
			}
			else {
				g_font->DrawText( nullptr, _T( "��]���v�Z�ł��Ȃ�Skeleton������܂�" ), -1, &textRect, 0, 0xFFFFFFAA );
				textRect.top += DEBUG_FONT_SIZE;
			}
		}
	}

	// ������̋�����\������
	if( g_skeleFrame.vFloorClipPlane.w > FLT_EPSILON ) {
		std::stringstream bufss;
		const Vector4 f = g_skeleFrame.vFloorClipPlane;
		bufss << "Kinect�̏�����̋��� : " << f.w << "[m]";
		g_font->DrawTextA( nullptr, bufss.str().c_str(), -1, &textRect, 0, 0xFFFFFFFF );
		textRect.top += DEBUG_FONT_SIZE;
	}
	// �������o�ł��Ȃ������Ƃ��A���̎|��\������
	else {
		g_font->DrawText( nullptr,
			_T( "�������o�ł��܂���D�J�����̈ʒu��ς��Ă�������" ), -1, &textRect, 0, 0xFFFFFFAA );
		textRect.top += DEBUG_FONT_SIZE;
	}

	// �`�悷��Skeleton�������Ȃ������Ƃ��A���̎|��\������
	if( numTrackedSkele == 0 ) {
		g_font->DrawText( nullptr, _T( "Skeleton�����o�ł��܂���" ), -1, &textRect, 0, 0xFFFFFFAA );
		textRect.top += DEBUG_FONT_SIZE;
	}
	// ����Ƃ��́A���̐���\������
	else {
		std::stringstream bufss;
		bufss << "Skeleton�̐� : " << numTrackedSkele;
		g_font->DrawTextA( nullptr, bufss.str().c_str(), -1, &textRect, 0, 0xFFAAFFFF );
		textRect.top += DEBUG_FONT_SIZE;
	}

	/* RGB�摜�̕\�� */
	
	// �[�x�o�b�t�@�[���g��Ȃ�
	g_d3ddev->SetRenderState( D3DRS_ZENABLE, FALSE );

	// ���_�̐ݒ�
	g_d3ddev->SetFVF( D3DFVF_XYZ | D3DFVF_TEX1 );
	g_d3ddev->SetStreamSource( 0, g_planeVB, 0, sizeof( FvfTex ) );

	// �e�N�X�`���[�̐ݒ�
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_d3ddev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	g_d3ddev->SetTexture( 0, g_kinectRgbTex );

	// ���W�ϊ��̐ݒ�i�ϊ����s��Ȃ��j
	D3DXMatrixIdentity( &matWorld );
	D3DXMatrixIdentity( &matView );
	D3DXMatrixIdentity( &matProj );
	g_d3ddev->SetTransform( D3DTS_WORLD, &matWorld );
	g_d3ddev->SetTransform( D3DTS_VIEW, &matView );
	g_d3ddev->SetTransform( D3DTS_PROJECTION, &matProj );

	// �r���[�|�[�g�̐ݒ�
	viewport.X      = WINDOW_WIDTH * 3 / 4;
	viewport.Width  = WINDOW_WIDTH * 1 / 4;
	viewport.Y      = 0;
	viewport.Height = WINDOW_HEIGHT * 1 / 4;
	g_d3ddev->SetViewport( &viewport );
	
	// �`��
	g_d3ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );

	/* �t���[�����[�g�̕\�� */

	// �r���[�|�[�g�̐ݒ�
	viewport.X      = 0;
	viewport.Width  = WINDOW_WIDTH;
	viewport.Y      = 0;
	viewport.Height = WINDOW_HEIGHT;
	g_d3ddev->SetViewport( &viewport );

	// �t���[�����[�g�̎擾�ƕ`��
	std::stringstream ssFps;
	int fps = updateFps();
	ssFps << "FPS : " << fps;
	textRect.top = 0;
	g_font->DrawTextA( nullptr, ssFps.str().c_str(), -1, &textRect, 0, 0xFFFFFFFF );

	/* �\������� */

	// �`����I������
	g_d3ddev->EndScene();

	// �f�B�X�v���C�ɓ]������
	hResult = g_d3ddev->Present( nullptr, nullptr, nullptr, nullptr );
	if( hResult != D3DERR_DEVICELOST && FAILED( hResult ) ) {
		throw d3d_exception( "Error : IDirect3DDevice9#Present" );
	}
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message ) {
	case WM_KEYDOWN:
		// OpenCV��highgui�̋����ɍ��킹��
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

// �E�B���h�E���쐬����
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

// �G���g���[�|�C���g
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
	MSG msg;
	ZeroMemory( &msg, sizeof msg );

	// Debug�ł͗�O���J�������󂯎��CRelease�ł̓��b�Z�[�W��\�����ċ����I�ɏI������
#ifdef NDEBUG
	try { 
#endif
	
	g_mainWindowHandle = setupWindow( WINDOW_WIDTH, WINDOW_HEIGHT );
	ShowWindow( g_mainWindowHandle, SW_SHOW );
	UpdateWindow( g_mainWindowHandle );

	// Kinect��Direct3D�̏���
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
		MessageBoxA( g_mainWindowHandle, e.what(), "Kinect Sample - ��肪�������܂���", MB_ICONSTOP ); }
#endif

	// Kinect��Direct3D�̏I��
	releaseKinect();
	releaseD3D();

	return static_cast<int>( msg.wParam );
}

