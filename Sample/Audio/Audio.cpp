// Audio.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

// Audio関連のヘッダーとライブラリ
#include <dmo.h>
#include <mfapi.h>
#include <wmcodecdsp.h>
#include <uuids.h>
#pragma comment( lib, "Msdmo.lib" )
#pragma comment( lib, "dmoguids.lib" )
#pragma comment( lib, "amstrmid.lib" )

#define _USE_MATH_DEFINES
#include <math.h>


// 四捨五入
int round(double degree)
{
	return degree < 0.0f ? static_cast<int>( std::ceil( degree - 0.5f ) ) : static_cast<int>( std::floor( degree + 0.5f ) );
}

// 結果描画
void drowResult(cv::Mat &img, const double beam, const double angle, const double confidence)
{
	int x = img.cols / 2;
	double length = 240;
	double baseAngle = 90.0f * M_PI / 180.0f;

	img = cv::Scalar( 255, 255, 255 ); // 描画画像の初期化

	// 目盛の描画
	cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -baseAngle ) ), static_cast<int>( length * -std::sin( -baseAngle ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA );
	for( int degree = 0; degree <= 50; degree += 10 ){
		cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -( baseAngle + ( degree * M_PI / 180.0f ) ) ) ), static_cast<int>( length * -std::sin( -( baseAngle + ( degree * M_PI / 180.0f ) ) ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA ); 
		cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -( baseAngle - ( degree * M_PI / 180.0f ) ) ) ), static_cast<int>( length * -std::sin( -( baseAngle - ( degree * M_PI / 180.0f ) ) ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA );
	}

	// 音源方向の描画
	cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( beam - baseAngle ) ), static_cast<int>( length * -std::sin( beam - baseAngle ) ) ), cv::Scalar( 0, 0, 255 ), 5, CV_AA ); // Beamforming

	// 方向、角度、信頼値の描画
	std::ostringstream stream;
	stream << round( (beam * 180.0f) / M_PI );
	cv::putText( img, "beam : " + stream.str(), cv::Point( 20, 300 ), cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar( 0, 0, 0 ), 1, CV_AA );

	stream.str("");
	stream << (angle * 180.0f) / M_PI;
	cv::putText( img, "angle : " + stream.str(), cv::Point( 20, 350 ), cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar( 0, 0, 0 ), 1, CV_AA );

	stream.str("");
	stream << confidence;
	cv::putText( img, "confidence : " + stream.str(), cv::Point( 20, 400 ), cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar( 0, 0, 0 ), 1, CV_AA );
}

// Kinect for Windows Developer Toolkit v1.6 - Samples/C++/AudioBasics-D2Dより引用
// This variable is: Copyright (c) Microsoft Corporation.  All rights reserved.
static const WORD AudioFormat = WAVE_FORMAT_PCM; // Format of Kinect audio stream
static const WORD AudioChannels = 1; // Number of channels in Kinect audio stream
static const DWORD AudioSamplesPerSecond = 16000; // Samples per second in Kinect audio stream
static const DWORD AudioAverageBytesPerSecond = 32000; // Average bytes per second in Kinect audio stream
static const WORD AudioBlockAlign = 2; // Block alignment in Kinect audio stream
static const WORD AudioBitsPerSample = 16; // Bits per audio sample in Kinect audio stream

// This class is: Copyright (c) Microsoft Corporation.  All rights reserved.
// IMediaBuffer implementation for a statically allocated buffer.
class CStaticMediaBuffer : public IMediaBuffer
{
public:
	// Constructor
	CStaticMediaBuffer() : m_dataLength(0) {}

	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IUnknown)
		{
			AddRef();
			*ppv = (IUnknown*)this;
			return NOERROR;
		}
		else if (riid == IID_IMediaBuffer)
		{
			AddRef();
			*ppv = (IMediaBuffer*)this;
			return NOERROR;
		}
		else
		{
			return E_NOINTERFACE;
		}
	}

	// IMediaBuffer methods
	STDMETHODIMP SetLength(DWORD length) {m_dataLength = length; return NOERROR;}
	STDMETHODIMP GetMaxLength(DWORD *pMaxLength) {*pMaxLength = sizeof(m_pData); return NOERROR;}
	STDMETHODIMP GetBufferAndLength(BYTE **ppBuffer, DWORD *pLength)
	{
		if (ppBuffer)
		{
			*ppBuffer = m_pData;
		}
		if (pLength)
		{
			*pLength = m_dataLength;
		}
		return NOERROR;
	}
	void Init(ULONG ulData)
	{
		m_dataLength = ulData;
	}

protected:
	// Statically allocated buffer used to hold audio data returned by IMediaObject
	BYTE m_pData[AudioSamplesPerSecond * AudioBlockAlign];

	// Amount of data currently being held in m_pData
	ULONG m_dataLength;
};

int _tmain(int argc, _TCHAR* argv[])
{
	cv::setUseOptimized( true );

	// Kinectのインスタンス生成、初期化
	INuiSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = NuiCreateSensorByIndex( 0, &pSensor );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiCreateSensorByIndex" << std::endl;
		return -1;
	}

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_AUDIO );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// 音源方向を推定するKinectの取得
	INuiAudioBeam* pNuiAudioBeam;
	hResult = pSensor->NuiGetAudioSource( &pNuiAudioBeam );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetAudioSource" << std::endl;
		return -1;
	}

	// 各種インターフェースの取得
	IMediaObject* pDMO = nullptr; // DMOインターフェース
	IPropertyStore* pPS = nullptr; // Property設定インターフェース
	pNuiAudioBeam->QueryInterface( IID_IMediaObject, reinterpret_cast<void**>( &pDMO ) );
	pNuiAudioBeam->QueryInterface( IID_IPropertyStore, reinterpret_cast<void**>( &pPS ) );

	// Microphoneアレイの設定
	PROPVARIANT pvSystemMode;
	PropVariantInit( &pvSystemMode );
	pvSystemMode.vt = VT_I4;
	pvSystemMode.lVal = static_cast<LONG>( 4 );
	pPS->SetValue( MFPKEY_WMAAECMA_SYSTEM_MODE, pvSystemMode );
	PropVariantClear( &pvSystemMode );

	// 取得するAudioデータの設定
	WAVEFORMATEX waveFormat = { AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0 };
	DMO_MEDIA_TYPE mediaType = { 0 };
	MoInitMediaType( &mediaType, sizeof( WAVEFORMATEX ) );

	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_PCM;
	mediaType.lSampleSize = 0;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = FALSE;
	mediaType.formattype = FORMAT_WaveFormatEx;
	memcpy( mediaType.pbFormat, &waveFormat, sizeof( WAVEFORMATEX ) );

	pDMO->SetOutputType( 0, &mediaType, 0 ); 
	MoFreeMediaType( &mediaType );

	// Audioバッファの確保
	CStaticMediaBuffer mediaBuffer;
	DMO_OUTPUT_DATA_BUFFER OutputBufferStruct = { 0 };
	OutputBufferStruct.pBuffer = &mediaBuffer;

	DWORD dwStatus = 0;

	cv::Mat audioMat( 480, 640, CV_8UC3 );
	cv::namedWindow( "Audio", cv::WINDOW_AUTOSIZE );

	while( 1 ){
		do{
			// Audio出力
			mediaBuffer.Init( 0 );
			OutputBufferStruct.dwStatus = 0;

			hResult = pDMO->ProcessOutput( 0, 1, &OutputBufferStruct, &dwStatus );
			if( SUCCEEDED(hResult) ){
				// 音源方向の取得
				double beam = 0.0f;
				double angle = 0.0f;
				double confidence = 0.0f;
				pNuiAudioBeam->GetBeam( &beam ); // -50度(-0.875ラジアン)～0度(0ラジアン)～+50度(+0.875ラジアン)の範囲で10度(0.175ラジアン)刻み11本のBeam．単位はラジアン．※おおまか
				pNuiAudioBeam->GetPosition( &angle, &confidence ); // DMOによって推定された音源方向の角度とその推定の信頼値(0.0～1.0)．単位はラジアン．※こまかく

				// 表示
				std::cout << "Beamforming beam : " << round( (beam * 180.0f) / M_PI ) << ", angle : " << (angle * 180.0f) / M_PI << ", confidence : " << confidence << std::endl;
				drowResult( audioMat, beam, angle, confidence );
			}
			else if( hResult != S_FALSE ){
				std::cerr << "Error : IMediaObject::ProcessOutput()" << std::endl;
				return -1;
			}
		}while( OutputBufferStruct.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE );

		cv::imshow( "Audio", audioMat );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinectの終了処理
	pDMO->Release();
	pPS->Release();
	pNuiAudioBeam->Release();
	pSensor->NuiShutdown();

	cv::destroyAllWindows();

	return 0;
}

