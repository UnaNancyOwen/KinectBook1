// Skeleton.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
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

	hResult = pSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON );
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

	// Skeletonストリーム
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiSkeletonTrackingEnable( hSkeletonEvent, 0 );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiSkeletonTrackingEnable" << std::endl;
		return -1;
	}

	HANDLE hEvents[3] = { hColorEvent, hDepthPlayerEvent, hSkeletonEvent };

	// カラーテーブル
	cv::Vec3b color[7];
	color[0] = cv::Vec3b(   0,   0,   0 );
	color[1] = cv::Vec3b( 255,   0,   0 );
	color[2] = cv::Vec3b(   0, 255,   0 );
	color[3] = cv::Vec3b(   0,   0, 255 );
	color[4] = cv::Vec3b( 255, 255,   0 );
	color[5] = cv::Vec3b( 255,   0, 255 );
	color[6] = cv::Vec3b(   0, 255, 255 );

	cv::namedWindow( "Color" );
	cv::namedWindow( "Depth" );
	cv::namedWindow( "Player" );
	cv::namedWindow( "Skeleton" );

	while( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
		ResetEvent( hSkeletonEvent );
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

		// Skeletonフレームを取得
		NUI_SKELETON_FRAME pSkeletonFrame = { 0 };
		hResult = pSensor->NuiSkeletonGetNextFrame( 0, &pSkeletonFrame );
		if( FAILED( hResult ) ){
			std::cout << "Error : NuiSkeletonGetNextFrame" << std::endl;
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

		// 表示
		cv::Mat colorMat( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );

		LONG registX = 0;
		LONG registY = 0;
		ushort* pBuffer = reinterpret_cast<ushort*>( sDepthPlayerLockedRect.pBits );
		cv::Mat bufferMat = cv::Mat::zeros( 480, 640, CV_16UC1 );
		cv::Mat playerMat = cv::Mat::zeros( 480, 640, CV_8UC3 );
		for( int y = 0; y < 480; y++ ){
			for( int x = 0; x < 640; x++ ){
				pSensor->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, nullptr, x, y, *pBuffer, &registX, &registY );
				if( ( registX >= 0 ) && ( registX < 640 ) && ( registY >= 0 ) && ( registY < 480 ) ){
					bufferMat.at<ushort>( registY, registX ) = *pBuffer & 0xFFF8;
					playerMat.at<cv::Vec3b>( registY, registX ) = color[*pBuffer & 0x7];
				}
				pBuffer++;
			}
		}
		cv::Mat depthMat( 480, 640, CV_8UC1 );
		bufferMat.convertTo( depthMat, CV_8UC3, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM, 255.0f );

		cv::Mat skeletonMat = cv::Mat::zeros( 480, 640, CV_8UC3 );
		cv::Point2f point;
		for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
			NUI_SKELETON_DATA skeleton = pSkeletonFrame.SkeletonData[count];
			if( skeleton.eTrackingState == NUI_SKELETON_TRACKED ){
				for( int position = 0; position < NUI_SKELETON_POSITION_COUNT; position++ ){
					NuiTransformSkeletonToDepthImage( skeleton.SkeletonPositions[position], &point.x, &point.y, NUI_IMAGE_RESOLUTION_640x480 );
					cv::circle( skeletonMat, point, 10, static_cast<cv::Scalar>( color[count + 1] ), -1, CV_AA );
				}
			}
		}
		
		cv::imshow( "Color", colorMat );
		cv::imshow( "Depth", depthMat );
		cv::imshow( "Player", playerMat );
		cv::imshow( "Skeleton", skeletonMat );

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
	pSensor->NuiSkeletonTrackingDisable();
	CloseHandle( hColorEvent );
	CloseHandle( hDepthPlayerEvent );
	CloseHandle( hSkeletonEvent );
	CloseHandle( hColorHandle );
	CloseHandle( hDepthPlayerHandle );

	cv::destroyAllWindows();

	return 0;
}

