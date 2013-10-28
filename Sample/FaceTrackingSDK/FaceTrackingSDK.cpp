// FaceTrackingSDK.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
//

#include "stdafx.h"
#include <Windows.h>
#include <NuiApi.h>
#include <FaceTrackLib.h>
#include <opencv2/opencv.hpp>


// Kinect for Windows Developer Toolkit v1.6 - Samples/C++/FaceTrackingVisualizationより引用(一部改変)
// This function is: Copyright (c) Microsoft Corporation.  All rights reserved.
HRESULT VisualizeFaceModel(IFTImage* pColorImg, IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef, 
	FLOAT zoomFactor, POINT viewOffset, IFTResult* pAAMRlt, UINT32 color)
{
	if (!pColorImg || !pModel || !pCameraConfig || !pSUCoef || !pAAMRlt)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	UINT vertexCount = pModel->GetVertexCount();
	FT_VECTOR2D* pPts2D = reinterpret_cast<FT_VECTOR2D*>(_malloca(sizeof(FT_VECTOR2D) * vertexCount));
	if (pPts2D)
	{
		FLOAT *pAUs;
		UINT auCount;
		hr = pAAMRlt->GetAUCoefficients(&pAUs, &auCount);
		if (SUCCEEDED(hr))
		{
			FLOAT scale, rotationXYZ[3], translationXYZ[3];
			hr = pAAMRlt->Get3DPose(&scale, rotationXYZ, translationXYZ);
			if (SUCCEEDED(hr))
			{
				hr = pModel->GetProjectedShape(pCameraConfig, zoomFactor, viewOffset, pSUCoef, pModel->GetSUCount(), pAUs, auCount, 
					 scale, rotationXYZ, translationXYZ, pPts2D, vertexCount);
				if (SUCCEEDED(hr))
				{
					POINT* p3DMdl   = reinterpret_cast<POINT*>(_malloca(sizeof(POINT) * vertexCount));
					if (p3DMdl)
					{
						for (UINT i = 0; i < vertexCount; ++i)
						{
							p3DMdl[i].x = LONG(pPts2D[i].x + 0.5f);
							p3DMdl[i].y = LONG(pPts2D[i].y + 0.5f);
						}

						FT_TRIANGLE* pTriangles;
						UINT triangleCount;
						hr = pModel->GetTriangles(&pTriangles, &triangleCount);
						if (SUCCEEDED(hr))
						{
							struct EdgeHashTable
							{
								UINT32* pEdges;
								UINT edgesAlloc;

								void Insert(int a, int b) 
								{
									UINT32 v = (std::min(a, b) << 16) | std::max(a, b);
									UINT32 index = (v + (v << 8)) * 49157, i;
									for (i = 0; i < edgesAlloc - 1 && pEdges[(index + i) & (edgesAlloc - 1)] && v != pEdges[(index + i) & (edgesAlloc - 1)]; ++i)
									{
									}
									pEdges[(index + i) & (edgesAlloc - 1)] = v;
								}
							} eht;

							eht.edgesAlloc = 1 << UINT(log(2.f * (1 + vertexCount + triangleCount)) / log(2.f));
							eht.pEdges = reinterpret_cast<UINT32*>(_malloca(sizeof(UINT32) * eht.edgesAlloc));
							if (eht.pEdges)
							{
								ZeroMemory(eht.pEdges, sizeof(UINT32) * eht.edgesAlloc);
								for (UINT i = 0; i < triangleCount; ++i)
								{ 
									eht.Insert(pTriangles[i].i, pTriangles[i].j);
									eht.Insert(pTriangles[i].j, pTriangles[i].k);
									eht.Insert(pTriangles[i].k, pTriangles[i].i);
								}
								for (UINT i = 0; i < eht.edgesAlloc; ++i)
								{
									if(eht.pEdges[i] != 0)
									{
										pColorImg->DrawLine(p3DMdl[eht.pEdges[i] >> 16], p3DMdl[eht.pEdges[i] & 0xFFFF], color, 1);
									}
								}
								_freea(eht.pEdges);
							}
						}
						_freea(p3DMdl); 
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
			}
		}
		_freea(pPts2D);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

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
		std::cerr << "Error : NuiImageStreamOpen(camera)" << std::endl;
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

	// Near Modeの設定
	hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
		return -1;
	}

	// Skeletonストリーム
	HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
	hSkeletonEvent = CreateEvent( nullptr, true, false, nullptr );
	hResult = pSensor->NuiSkeletonTrackingEnable( hSkeletonEvent, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA | NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT );
	if( FAILED( hResult ) ){
		std::cerr << "Error : NuiSkeletonTrackingEnable" << std::endl;
		return -1;
	}
 
	// Face Tracking初期化
	IFTFaceTracker* pFT = FTCreateFaceTracker();
	if( !pFT ){
		std::cerr << "Error : FTCreateFaceTracker" << std::endl;
		return -1;
	}

	FT_CAMERA_CONFIG colorConfig  = { 640, 480, NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS };
	FT_CAMERA_CONFIG depthConfig  = { 640, 480, NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.0f };
	hResult = pFT->Initialize( &colorConfig, &depthConfig, nullptr, nullptr );
	if( FAILED( hResult ) ){
		std::cerr << "Error : Initialize" << std::endl;
		return -1;
	}

	// Face Trackingの結果インターフェース
	IFTResult* pFTResult = nullptr;
	hResult = pFT->CreateFTResult( &pFTResult );
	if( FAILED( hResult )  || !pFTResult ){
		std::cerr << "Error : CreateFTResult" << std::endl;
		return -1;
	}

	// Face Trackingのための画像
	IFTImage* pColorImage = FTCreateImage();
	IFTImage* pDepthImage = FTCreateImage();
	if( !pColorImage || !pDepthImage )
	{
		std::cerr << "Error : FTCreateImage" << std::endl;
		return -1;
	}
	pColorImage->Allocate( 640, 480, FTIMAGEFORMAT_UINT8_B8G8R8X8 );
	pDepthImage->Allocate( 640, 480, FTIMAGEFORMAT_UINT16_D13P3 );

	FT_VECTOR3D* hintPoint = nullptr;
	bool lastTrack = false;

	HANDLE hEvents[3] = { hColorEvent, hDepthPlayerEvent, hSkeletonEvent };

	cv::namedWindow( "Face Tracking" );
	cv::namedWindow( "Depth" );

	while ( 1 ){
		// フレームの更新待ち
		ResetEvent( hColorEvent );
		ResetEvent( hDepthPlayerEvent );
		ResetEvent( hSkeletonEvent );
		WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, true, INFINITE );

		// Colorカメラからフレームを取得
		NUI_IMAGE_FRAME sColorImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hColorHandle, 0, &sColorImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
			return -1;
		}

		// Depthセンサーからフレームを取得
		NUI_IMAGE_FRAME sDepthPlayerImageFrame = { 0 };
		hResult = pSensor->NuiImageStreamGetNextFrame( hDepthPlayerHandle, 0, &sDepthPlayerImageFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH&PLAYER )" << std::endl;
			return -1;
		}

		// Skeletonフレームを取得
		NUI_SKELETON_FRAME sSkeletonFrame = { 0 };
		hResult = pSensor->NuiSkeletonGetNextFrame( 0, &sSkeletonFrame );
		if( FAILED( hResult ) ){
			std::cerr << "Error : NuiSkeletonGetNextFrame" << std::endl;
			return -1;
		}

		// Color画像データの取得
		INuiFrameTexture* pColorTexture = sColorImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sColorLockedRect;
		pColorTexture->LockRect( 0, &sColorLockedRect, nullptr, 0 );
		cv::Mat colorMat( 480, 640, CV_8UC4, reinterpret_cast<uchar*>( sColorLockedRect.pBits ) );

		memcpy( pColorImage->GetBuffer(), PBYTE(sColorLockedRect.pBits), std::min( pColorImage->GetBufferSize(), UINT(pColorTexture->BufferLen()) ) ); // Face Trackingのための画像へコピー

		// Depthデータの取得
		INuiFrameTexture* pDepthPlayerTexture = sDepthPlayerImageFrame.pFrameTexture;
		NUI_LOCKED_RECT sDepthPlayerLockedRect;
		pDepthPlayerTexture->LockRect( 0, &sDepthPlayerLockedRect, nullptr, 0 );
		LONG registX = 0;
		LONG registY = 0;
		ushort* pBuffer = reinterpret_cast<ushort*>( sDepthPlayerLockedRect.pBits );
		cv::Mat bufferMat16U = cv::Mat::zeros( 480, 640, CV_16UC1 );
		for( int y = 0; y < 480; y++ ){
			for( int x = 0; x < 640; x++ ){
				pSensor->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, nullptr, x, y, *pBuffer, &registX, &registY );
				if( ( registX >= 0 ) && ( registX < 640 ) && ( registY >= 0 ) && ( registY < 480 ) ){
					bufferMat16U.at<ushort>( registY, registX ) = *pBuffer & 0xFFF8;
				}
				pBuffer++;
			}
		}
		cv::Mat bufferMat8U( 480, 640, CV_8UC1 );
		bufferMat16U.convertTo( bufferMat8U, CV_8UC1, -255.0f / NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE, 255.0f );
		cv::Mat depthMat( 480, 640, CV_8UC3 );
		cv::cvtColor( bufferMat8U, depthMat, CV_GRAY2BGR );

		memcpy( pDepthImage->GetBuffer(), PBYTE(sDepthPlayerLockedRect.pBits), std::min( pDepthImage->GetBufferSize(), UINT(pDepthPlayerTexture->BufferLen()) ) ); // Face Trackingのための画像へコピー
		
		// Skeletonデータ(頭、首)の取得
		bool skeletonTracked[NUI_SKELETON_COUNT];
		FT_VECTOR3D neckPoint[NUI_SKELETON_COUNT];
		FT_VECTOR3D headPoint[NUI_SKELETON_COUNT];
		for( int count = 0; count < NUI_SKELETON_COUNT; count++ ){
			headPoint[count] = neckPoint[count] = FT_VECTOR3D(0, 0, 0);
			skeletonTracked[count] = false;
			NUI_SKELETON_DATA skeleton = sSkeletonFrame.SkeletonData[count];
			if( skeleton.eTrackingState == NUI_SKELETON_TRACKED ){
				skeletonTracked[count] = true;
				if( skeleton.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HEAD] != NUI_SKELETON_POSITION_NOT_TRACKED ){
					headPoint[count].x = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x;
					headPoint[count].y = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y;
					headPoint[count].z = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
				}
				if( skeleton.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED ){
					neckPoint[count].x = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x;
					neckPoint[count].y = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y;
					neckPoint[count].z = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z;
				}
			}
		}

		// 一番近いPlayerのSkleton(頭、首)をヒントとして与える
		int selectedSkeleton = -1;
		float smallestDistance = 0.0f;
		if( hintPoint == nullptr ){
			for( int count = 0 ; count < NUI_SKELETON_COUNT ; count++ ){
				if( skeletonTracked[count] && ( smallestDistance == 0 || headPoint[count].z < smallestDistance ) ){
					smallestDistance = headPoint[count].z;
					selectedSkeleton = count;
				}
			}
		}
		else{
			for( int count = 0 ; count < NUI_SKELETON_COUNT ; count++ ){
				if( skeletonTracked[count] ){
					float distance = std::abs( headPoint[count].x - hintPoint[1].x ) + std::abs( headPoint[count].y - hintPoint[1].y ) + std::abs( headPoint[count].z - hintPoint[1].z );
					if( smallestDistance == 0.0f || distance < smallestDistance ){
						smallestDistance = distance;
						selectedSkeleton = count;
					}
				}
			}
		}

		if( selectedSkeleton != -1 ){
			FT_VECTOR3D hint[2];
			hint[0] = neckPoint[selectedSkeleton];
			hint[1] = headPoint[selectedSkeleton];
			hintPoint = hint;

			FLOAT depthX[2] = { 0 };
			FLOAT depthY[2] = { 0 };
			NUI_SKELETON_DATA hintSkeleton = sSkeletonFrame.SkeletonData[selectedSkeleton];
			NuiTransformSkeletonToDepthImage( hintSkeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], &depthX[0], &depthY[0], NUI_IMAGE_RESOLUTION_640x480 );
			NuiTransformSkeletonToDepthImage( hintSkeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER], &depthX[1], &depthY[1], NUI_IMAGE_RESOLUTION_640x480 );
			cv::circle( depthMat, cv::Point( static_cast<int>( depthX[0] ), static_cast<int>( depthY[0] ) ), 10, cv::Scalar( 0, 0, 255 ), -1, CV_AA );
			cv::circle( depthMat, cv::Point( static_cast<int>( depthX[1] ), static_cast<int>( depthY[1] ) ), 10, cv::Scalar( 0, 0, 255 ), -1, CV_AA );
		}
		else{
			hintPoint = nullptr;
		}

		// センサーデータを設定する
		FT_SENSOR_DATA sensorData;
		sensorData.pVideoFrame = pColorImage;
		sensorData.pDepthFrame = pDepthImage;
		sensorData.ZoomFactor = 1.0f;
		POINT viewOffset = { 0, 0 };
		sensorData.ViewOffset = viewOffset;

		// FaceTrackingの検出・追跡 
		if( lastTrack ){
			// This method is faster than StartTracking() and is used only for tracking. But, If the face being tracked moves too far from the previous location, this method fails.
			hResult = pFT->ContinueTracking( &sensorData, hintPoint, pFTResult );
			if( FAILED( hResult ) || FAILED( pFTResult->GetStatus() ) ){
				lastTrack = false;
			}
		}
		else{
			// This process is more expensive than simply tracking (done by calling ContinueTracking()), but more robust.
			hResult = pFT->StartTracking( &sensorData, nullptr, hintPoint, pFTResult );
			if( SUCCEEDED( hResult ) && SUCCEEDED( pFTResult->GetStatus() ) ){
				lastTrack = true;
			}
			else{
				lastTrack = false;
			}
		}

		// Face Tracking結果描画
		if( lastTrack && SUCCEEDED( pFTResult->GetStatus() ) ){
			// Candide-3 Face Model(http://www.icg.isy.liu.se/candide/)の表示
			IFTModel* pFTModel;
			hResult = pFT->GetFaceModel( &pFTModel );
			if( SUCCEEDED( hResult ) ){
				FLOAT* pSU = nullptr;
				pFT->GetShapeUnits( nullptr, &pSU, nullptr, nullptr ); 
				VisualizeFaceModel( pColorImage, pFTModel, &colorConfig, pSU, 1.0f, viewOffset, pFTResult, 0x00FF0000 ); // ARGB Red:0x00FF0000, Grean:0x0000FF00, Blue:0x000000FF, Yellow:0x00FFFF00
				pFTModel->Release();
				colorMat.data = reinterpret_cast<uchar*>( pColorImage->GetBuffer() );
			}
		}

		cv::imshow( "Face Tracking", colorMat );
		cv::imshow( "Depth", depthMat );

		// フレームの解放
		pColorTexture->UnlockRect( 0 );
		pDepthPlayerTexture->UnlockRect( 0 );
		pSensor->NuiImageStreamReleaseFrame( hColorHandle, &sColorImageFrame );
		pSensor->NuiImageStreamReleaseFrame( hDepthPlayerHandle, &sDepthPlayerImageFrame );

		// ループの終了判定(Escキー)
		if( cv::waitKey( 30 ) == VK_ESCAPE ){
			break;
		}
	}

	// Kinectの終了処理
	pFT->Release();
	pFTResult->Release();
	pColorImage->Release();
	pDepthImage->Release();
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

