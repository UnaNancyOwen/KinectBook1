書籍「Kinect for Windows SDK プログラミングブック」サンプルプログラム ReadMe.txt

■構成
本サンプルプログラムの構成は以下のとおりです。

    Sample.zip
    │
    │  // サンプルプログラム
    ├─Sample
    │  │
    │  ├─Sample.sln
    │  │
    │  ├─Color
    │  │  ├─Color.vcxproj
    │  │  └─Color.cpp
    │  │
    │  ├─Depth
    │  │  ├─Depth.vcxproj
    │  │  └─Depth.cpp
    │  │
    │  ├─Player
    │  │  ├─Player.vcxproj
    │  │  └─Player.cpp
    │  │
    │  ├─Skeleton
    │  │  ├─Skeleton.vcxproj
    │  │  └─Skeleton.cpp
    │  │
    │  ├─Audio
    │  │  ├─Audio.vcxproj
    │  │  └─Audio.cpp
    │  │
    │  ├─Clipping
    │  │  ├─Clipping.vcxproj
    │  │  └─Clipping.cpp
    │  │
    │  ├─MotionCapture
    │  │  ├─MotionCapture.vcxproj
    │  │  └─MotionCapture.cpp
    │  │
    │  └─FaceTrackingSDK
    │      ├─FaceTrackingSDK.vcxproj
    │      └─FaceTrackingSDK.cpp
    │
    │  // プロパティシート
    ├─KinectBook.props
    │
    │  // ライセンス
    ├─License.txt
    │
    │  // 説明
    └─ReadMe.txt


■サンプルプログラムについて
本サンプルプログラムは以下の環境でビルドできるように設定しています。
環境によっては適宜設定を書き換えてください。

    Visual C++ 2010 Express
    Kinect for Windows SDK v1.6
    Kinect for Windows Developer Toolkit v1.6
    OpenCV 2.4.2
    DirectX SDK (June 2010)


■Kinectについて
Kinect for Xbox360ではNear Modeは動作しないため、ソースコードの該当箇所をコメントアウトしてください。

    /*
    // Near Modeの設定
    hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
    if( FAILED( hResult ) ){
        std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
        return -1;
    }
    */

    NUI_IMAGE_DEPTH_MAXIMUM/*_NEAR_MODE*/

また、Kinectが上手く動作しない場合はKinect for Windows Developer ToolkitのKinect Explorerで簡単に状況を確認することができます。
(これらはプログラム中でHRESULT型のエラーコードからでも確認することができます。)

    例：NotPowered → 電源が通っていない可能性があります。
                      USB/電源ケーブルの接続を確認してください。

USBコントローラーとの相性が原因である場合もあります。
別のUSBコントローラーで制御されているUSBポートに接続し直すと上手く動作することがあります。


■プロパティーシートについて
付属のプロパティシートKinectBook.propsには以下の設定が記述されています。
作成したプロジェクトの設定に役に立つと思います。
なお、本サンプルプログラムは付属のプロパティーシートに依存していません。

    Kinect for Windows SDK v1.6
    Kinect for Windows Developer Toolkit v1.6
    OpenCV 2.4.2


■OpenCVについて
OpenCVはバージョンによってライブラリ名が異なります。
現状は以下の命名規則でライブラリ名がつけられています。

Debug用のライブラリファイルには最後に「d」がつきます。

    Debug   ：opencv_◯◯◯△△△d.lib
    Release ：opencv_◯◯◯△△△.lib

△△△はバージョン番号です。

    例：OpenCV 2.4.2 → opencv_◯◯◯242.lib

◯◯◯はモジュール名です。
利用する機能にあわせてライブラリをリンクします。
よく使われるモジュールは以下のとおりです。

    core    ：基礎
    highgui ：ユーザーインターフェース
    imgproc ：画像処理

    例：core → opencv_core△△△.lib

インクルードするヘッダファイルは<opencv/opencv.hpp>です。
一部を除きほぼすべてのOpenCV関連のヘッダファイルがインクルードされます。

    #include <opencv/opencv.hpp>

OpenCV 2.4.3にはVisual Studio 2012(Visual Studio 11)向けのバイナリが同梱されていません．
OpenCV 2.4.3を使用する場合はCMakeを使ってVisual Studio 2012(Visual Studio 11またはVisual Studio 11 Win64)のソリューションを作成し，ビルドしてください．

    OpenCV v2.4.3 documentation > OpenCV Tutorials > Introduction to OpenCV > Installation in Windows > Building the library
    http://docs.opencv.org/trunk/doc/tutorials/introduction/windows_install/windows_install.html#building-the-library


■環境変数について
Kinect for Windows SDKおよびKinect for Windows Developer Toolkit、DirectX SDK (June 2010)をインストールすると以下の環境変数が設定されます。
本サンプルプログラムの各種設定は環境変数に依存しています。
今後のバージョンアップに伴い環境変数の変数名や値が変化する場合があるので適宜設定を書き換えてください。

    インストールで自動で設定される環境変数
        変数名          値
        KINECTSDK10_DIR C:\Program Files\Microsoft SDKs\Kinect\v1.6\
        FTSDK_DIR       C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.6.0\
        DXSDK_DIR       C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\

    手動で新規に作成する環境変数
        変数名          値
        OPENCV_DIR      C:\Program Files\opencv\build\

    手動で値を追加する環境変数(値の間は「;」で区切ります)
        変数名          値(Win32)
        Path            ;%FTSDK_DIR%Redist\x86;%OPENCV_DIR%x86\vc10\bin;%OPENCV_DIR%common\tbb\ia32\vc10
                        値(x64)
                        ;%FTSDK_DIR%Redist\amd64;%OPENCV_DIR%x64\vc10\bin;%OPENCV_DIR%common\tbb\intel64\vc10

また、環境変数で他の変数を参照する場合は%◯◯◯%と記述します。
Visual C++で環境変数を参照する場合は$(◯◯◯)と記述します。
それぞれ変数の値が展開されて認識されます。

    例：Face Tracking SDKのWin32実行ファイル(*.dll)のディレクトリを指定する場合
            環境変数   ： %FTSDK_DIR%Redist\x86 → C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.6.0\Redist\x86

        Kinect for Windows SDKのインクルード ディレクトリを指定する場合
            Visual C++ ： $(KINECTSDK10_DIR)inc → C:\Program Files\Microsoft SDKs\Kinect\v1.6\inc

環境変数の設定を間違えるとOSが起動しなくなる可能性もあるので注意してください。
また、環境変数を変更した場合はVisual C++を再起動して変更を反映してください。


■x64プラットフォームでのビルドについて
Visual C++ 2010 Expressは標準ではx64プラットフォームでビルドできません。
(Express以外のVisual Studio製品では標準でx64プラットフォームでビルドできます。)
Visual C++ 2010 Expressでx64プラットフォームでビルドするには、
Windows SDK 7.1をインストールして、プロジェクトのプロパティページの[構成プロパティ]>[全般]の「プラットフォームツールセット」を
「v100」から「Windows7.1SDK」に変更することでビルドできるようになります。

    Microsoft Windows SDK for Windows 7 and .NET Framework 4 (Windows SDK 7.1)
    http://www.microsoft.com/en-us/download/details.aspx?id=8279

x64プラットフォームでビルドするには、Visual C++のメニューから[ビルド]>[構成マネージャー]を選択し、「構成マネージャー」ウィンドウを表示します。
[アクティブ ソリューション プラットフォーム]から[<新規作成...>]を選択し、「新しいソリューション プラットフォーム」ウィンドウを表示します。
[新しいプラットフォームを入力または選択してください]のドロップダウンリストを押し、「64ビット プラットフォーム」を選択します。

プロジェクトの各種設定(ライブラリ ディレクトリなど)はx64の設定をしてください。
詳しくは以下のページを参照してください。

    MSDNライブラリ > Visual C++ > Visual C++による64ビットプログラミング > 方法 : Visual C++ プロジェクトを 64 ビット プラットフォーム用に設定する
    http://msdn.microsoft.com/ja-jp/library/9yb4317s(v=vs.100).aspx

なお、Visual Studio Express 2012 for Windows Desktopでは標準でx64プラットフォームでビルドできます。


■スタートアップ プロジェクトについて
スタートアップ プロジェクトは標準では「シングル スタートアップ プロジェクト」になっています。
実行するプロジェクトを変更するには、ソリューションのコンテキストメニューから[プロパティ]を選択、
[共有プロパティ]>[スタートアッププロジェクト]の「シングル スタートアップ プロジェクト」のドロップダウンメニューから実行したいプロジェクトを選択して設定してください。
また、[現在の選択]にチェックボックスを入れて設定すると、現在選択されているプロジェクトがスタートアップ プロジェクトになるので便利です。


■処理速度について
サンプルプログラムを動かす場合は「Release」構成でビルドしてください。
コンパイラの最適化が有効になります。


■動作確認
本サンプルプログラムは以下の環境で動作を確認しました。
本サンプルプログラムはすべての環境について動作を保証するものではありません。

    OS      ：Windows 7 SP1
    Sensor  ：Kinect for Windows
    IDE     ：Visual C++ 2010 Express SP1
    Library ：Kinect for Windows SDK v1.6
              Kinect for Developer Toolkit v1.6
              OpenCV 2.4.2
              DirectX SDK (June 2010)


■ライセンスについて
本サンプルプログラムのライセンスはMIT Licenseとします。
ライセンス文はLicense.txtを参照してください。


■免責事項
本サンプルプログラムは使用者の責任において利用してください。
このサンプルプログラムによって発生したいかなる障害・損害などに対して、著作権者は一切責任を負わないものとします。
また、本サンプルプログラムは予告なく変更や削除する場合があります。


■変更履歴
2012/11/19 サンプルプログラムを作成しました。
