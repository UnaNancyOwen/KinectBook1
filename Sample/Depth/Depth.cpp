// Depth.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>


int _tmain( int argc, _TCHAR* argv[] )
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Color�X�g���[��
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Depth�X�g���[��
	HANDLE hDepthEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthHandle = INVALID_HANDLE_VALUE;
	hDepthEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hDepthEvent, &hDepthHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( DEPTH )" << std::endl;
		return -1;
	}

	// Near Mode�̐ݒ�
	hResult = pSensor->NuiImageStreamSetImageFrameFlags(hDepthHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
		return -1;
	}

	HANDLE hEvents[2] = { hColorEvent, hDepthEvent };

	cv::namedWindow( "Color" );
	cv::namedWindow( "Depth" );

	while( 1 ){
		// �t���[���̍X�V�҂�
		ResetEvent( hColorEvent );
		ResetEvent( hDepthEvent );
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

		// Color�J��������t���[�����擾
		NUI_IMAGE_FRAME pColorImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hColorHandle, 0, &pColorImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return -1;
		}

		// Depth�Z���T�[����t���[�����擾
		NUI_IMAGE_FRAME pDepthImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthHandle, 0, &pDepthImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH )" << std::endl;
			return -1;
		}

		// Color�摜�f�[�^�̎擾
		INuiFrameTexture* pColorFrameTexture = pColorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sColorLockedRect;
		pColorFrameTexture->LockRect( 0, &sColorLockedRect, nullptr, 0 );

		// Depth�f�[�^�̎擾
		INuiFrameTexture* pDepthFrameTexture = pDepthImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sDepthLockedRect;
		pDepthFrameTexture->LockRect( 0, &sDepthLockedRect, nullptr, 0 );

		// �\��
		cv::Mat colorMat( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );
		LONG registX = 0;
		LONG registY = 0;
		ushort* pBuffer = reinterpret_cast<ushort*>( sDepthLockedRect.pBits );
		cv::Mat bufferMat = cv::Mat::zeros( 480, 640, CV_16UC1 );
		for( int y = 0; y < 480; y++ ){
			for( int x = 0; x < 640; x++ ){
				pSensor->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, nullptr, x, y, *pBuffer, &registX, &registY );
				if( ( registX >= 0 ) && ( registX < 640 ) && ( registY >= 0 ) && ( registY < 480 ) ){
					bufferMat.at<ushort>( registY, registX ) = *pBuffer;
				}
				pBuffer++;
			}
		}
		cv::Mat depthMat( 480, 640, CV_8UC1 );
		bufferMat.convertTo( depthMat, CV_8UC1, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE, 255.0f );
		cv::imshow( "Color", colorMat );
		cv::imshow( "Depth", depthMat );
		
		// �t���[���̉��
		pColorFrameTexture->UnlockRect( 0 );
		pDepthFrameTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &pColorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthHandle, &pDepthImageFrame );


		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinect�̏I������
	pSensor->NuiShutdown();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthEvent );
	CloseHandle( hColorHandle );
	CloseHandle( hDepthHandle );

	cv::destroyAllWindows();

	return 0;
}

