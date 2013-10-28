// Audio.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

// Audio�֘A�̃w�b�_�[�ƃ��C�u����
#include <dmo.h>
#include <mfapi.h>
#include <wmcodecdsp.h>
#include <uuids.h>
#pragma comment( lib, "Msdmo.lib" )
#pragma comment( lib, "dmoguids.lib" )
#pragma comment( lib, "amstrmid.lib" )

#define _USE_MATH_DEFINES
#include <math.h>


// �l�̌ܓ�
int round(double degree)
{
	return degree < 0.0f ? static_cast<int>( std::ceil( degree - 0.5f ) ) : static_cast<int>( std::floor( degree + 0.5f ) );
}

// ���ʕ`��
void drowResult(cv::Mat &img, const double beam, const double angle, const double confidence)
{
	int x = img.cols / 2;
	double length = 240;
	double baseAngle = 90.0f * M_PI / 180.0f;

	img = cv::Scalar( 255, 255, 255 ); // �`��摜�̏�����

	// �ڐ��̕`��
	cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -baseAngle ) ), static_cast<int>( length * -std::sin( -baseAngle ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA );
	for( int degree = 0; degree <= 50; degree += 10 ){
		cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -( baseAngle + ( degree * M_PI / 180.0f ) ) ) ), static_cast<int>( length * -std::sin( -( baseAngle + ( degree * M_PI / 180.0f ) ) ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA ); 
		cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( -( baseAngle - ( degree * M_PI / 180.0f ) ) ) ), static_cast<int>( length * -std::sin( -( baseAngle - ( degree * M_PI / 180.0f ) ) ) ) ), cv::Scalar( 0, 0, 0 ), 1, CV_AA );
	}

	// ���������̕`��
	cv::line( img, cv::Point( x, 0 ), cv::Point( x + static_cast<int>( length * std::cos( beam - baseAngle ) ), static_cast<int>( length * -std::sin( beam - baseAngle ) ) ), cv::Scalar( 0, 0, 255 ), 5, CV_AA ); // Beamforming

	// �����A�p�x�A�M���l�̕`��
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

// Kinect for Windows Developer Toolkit v1.6 - Samples/C++/AudioBasics-D2D�����p
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

	// Kinect�̃C���X�^���X�����A������
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

	// ���������𐄒肷��Kinect�̎擾
	INuiAudioBeam* pNuiAudioBeam;
	hResult = pSensor->NuiGetAudioSource( &pNuiAudioBeam );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiGetAudioSource" << std::endl;
		return -1;
	}

	// �e��C���^�[�t�F�[�X�̎擾
	IMediaObject* pDMO = nullptr; // DMO�C���^�[�t�F�[�X
	IPropertyStore* pPS = nullptr; // Property�ݒ�C���^�[�t�F�[�X
	pNuiAudioBeam->QueryInterface( IID_IMediaObject, reinterpret_cast<void**>( &pDMO ) );
	pNuiAudioBeam->QueryInterface( IID_IPropertyStore, reinterpret_cast<void**>( &pPS ) );

	// Microphone�A���C�̐ݒ�
	PROPVARIANT pvSystemMode;
	PropVariantInit( &pvSystemMode );
	pvSystemMode.vt = VT_I4;
	pvSystemMode.lVal = static_cast<LONG>( 4 );
	pPS->SetValue( MFPKEY_WMAAECMA_SYSTEM_MODE, pvSystemMode );
	PropVariantClear( &pvSystemMode );

	// �擾����Audio�f�[�^�̐ݒ�
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

	// Audio�o�b�t�@�̊m��
	CStaticMediaBuffer mediaBuffer;
	DMO_OUTPUT_DATA_BUFFER OutputBufferStruct = { 0 };
	OutputBufferStruct.pBuffer = &mediaBuffer;

	DWORD dwStatus = 0;

	cv::Mat audioMat( 480, 640, CV_8UC3 );
	cv::namedWindow( "Audio", cv::WINDOW_AUTOSIZE );

	while( 1 ){
		do{
			// Audio�o��
			mediaBuffer.Init( 0 );
			OutputBufferStruct.dwStatus = 0;

			hResult = pDMO->ProcessOutput( 0, 1, &OutputBufferStruct, &dwStatus );
			if( SUCCEEDED(hResult) ){
				// ���������̎擾
				double beam = 0.0f;
				double angle = 0.0f;
				double confidence = 0.0f;
				pNuiAudioBeam->GetBeam( &beam ); // -50�x(-0.875���W�A��)�`0�x(0���W�A��)�`+50�x(+0.875���W�A��)�͈̔͂�10�x(0.175���W�A��)����11�{��Beam�D�P�ʂ̓��W�A���D�������܂�
				pNuiAudioBeam->GetPosition( &angle, &confidence ); // DMO�ɂ���Đ��肳�ꂽ���������̊p�x�Ƃ��̐���̐M���l(0.0�`1.0)�D�P�ʂ̓��W�A���D�����܂���

				// �\��
				std::cout << "Beamforming beam : " << round( (beam * 180.0f) / M_PI ) << ", angle : " << (angle * 180.0f) / M_PI << ", confidence : " << confidence << std::endl;
				drowResult( audioMat, beam, angle, confidence );
			}
			else if( hResult != S_FALSE ){
				std::cerr << "Error : IMediaObject::ProcessOutput()" << std::endl;
				return -1;
			}
		}while( OutputBufferStruct.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE );

		cv::imshow( "Audio", audioMat );

		// ���[�v�̏I������(Esc�L�[)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinect�̏I������
	pDMO->Release();
	pPS->Release();
	pNuiAudioBeam->Release();
	pSensor->NuiShutdown();

	cv::destroyAllWindows();

	return 0;
}

