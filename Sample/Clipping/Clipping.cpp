// Clipping.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>


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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiInitialize" << std::endl;
		return -1;
	}

	// Colorストリーム
	HANDLE hColorEvent = INVALID_HANDLE_VALUE;
	HANDLE hColorHandle = INVALID_HANDLE_VALUE;
	hColorEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hColorEvent, &hColorHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( COLOR )" << std::endl;
		return -1;
	}

	// Depth&Playerストリーム
	HANDLE hDepthPlayerEvent = INVALID_HANDLE_VALUE;
	HANDLE hDepthPlayerHandle = INVALID_HANDLE_VALUE;
	hDepthPlayerEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480, 0, 2, hDepthPlayerEvent, &hDepthPlayerHandle );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamOpen( DEPTH&PLAYER )" << std::endl;
		return -1;
	}

	HANDLE hEvents[2] = { hColorEvent, hDepthPlayerEvent };

	cv::namedWindow( "Mask" );
	cv::namedWindow( "Clip" );

	// トラックバーの生成
	int iterationErode = 2;
	int iterationDilate = 2;
	cv::createTrackbar( "erode", "Mask", &iterationErode, 10 );
	cv::createTrackbar( "dilate", "Mask", &iterationDilate, 10 );

	while( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

		// Colorカメラからフレームを取得
		NUI_IMAGE_FRAME pColorImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hColorHandle, 0, &pColorImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return -1;
		}

		// Depthセンサーからフレームを取得
		NUI_IMAGE_FRAME pDepthPlayerImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &pDepthPlayerImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
			return -1;
		}

		// Color画像データの取得
		INuiFrameTexture* pColorFrameTexture = pColorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sColorLockedRect;
		pColorFrameTexture->LockRect( 0, &sColorLockedRect, nullptr, 0 );

		// Depthデータの取得
		INuiFrameTexture* pDepthPlayerFrameTexture = pDepthPlayerImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sDepthPlayerLockedRect;
		pDepthPlayerFrameTexture->LockRect( 0, &sDepthPlayerLockedRect, nullptr, 0 );

		// 画像の取得
		cv::Mat colorMat( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );
		LONG registX = 0;
		LONG registY = 0;
		ushort* pBuffer = reinterpret_cast<ushort*>( sDepthPlayerLockedRect.pBits );
		cv::Mat maskMat = cv::Mat::zeros( 480, 640, CV_8UC1 );
		for( int y = 0; y < 480; y++ ){
			for( int x = 0; x < 640; x++ ){
				pSensor->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, nullptr, x, y, *pBuffer, &registX, &registY );
				if( ( registX >= 0 ) && ( registX < 640 ) && ( registY >= 0 ) && ( registY < 480 ) ){
					if( ( *pBuffer & 0x7 ) != 0 ){
						maskMat.at<uchar>( registY, registX ) = 0xff; // 255
					}
				}
				pBuffer++;
			}
		}

		// 処理
		// Mathematical Morphology - opening
		cv::erode( maskMat, maskMat, cv::Mat(), cv::Point( -1, -1 ), iterationErode );
		cv::dilate( maskMat, maskMat, cv::Mat(), cv::Point( -1, -1 ), iterationDilate );

		// Mathematical Morphology - cpening
		cv::dilate( maskMat, maskMat, cv::Mat(), cv::Point( -1, -1 ), iterationDilate );
		cv::erode( maskMat, maskMat, cv::Mat(), cv::Point( -1, -1 ), iterationErode );
		
		cv::Mat clipMat = cv::Mat::zeros( 480, 640, CV_8UC4 );
		colorMat.copyTo( clipMat, maskMat );

		cv::imshow( "Mask", maskMat );
		cv::imshow( "Clip", clipMat );

		// フレームの解放
		pColorFrameTexture->UnlockRect( 0 );
		pDepthPlayerFrameTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &pColorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &pDepthPlayerImageFrame );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinectの終了処理
	pSensor->NuiShutdown();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hColorHandle );
	CloseHandle( hDepthPlayerHandle );

	cv::destroyAllWindows();

	return 0;
}

