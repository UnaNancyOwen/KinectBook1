���ЁuKinect for Windows SDK �v���O���~���O�u�b�N�v�T���v���v���O���� ReadMe.txt

���\��
�{�T���v���v���O�����̍\���͈ȉ��̂Ƃ���ł��B

    Sample.zip
    ��
    ��  // �T���v���v���O����
    ����Sample
    ��  ��
    ��  ����Sample.sln
    ��  ��
    ��  ����Color
    ��  ��  ����Color.vcxproj
    ��  ��  ����Color.cpp
    ��  ��
    ��  ����Depth
    ��  ��  ����Depth.vcxproj
    ��  ��  ����Depth.cpp
    ��  ��
    ��  ����Player
    ��  ��  ����Player.vcxproj
    ��  ��  ����Player.cpp
    ��  ��
    ��  ����Skeleton
    ��  ��  ����Skeleton.vcxproj
    ��  ��  ����Skeleton.cpp
    ��  ��
    ��  ����Audio
    ��  ��  ����Audio.vcxproj
    ��  ��  ����Audio.cpp
    ��  ��
    ��  ����Clipping
    ��  ��  ����Clipping.vcxproj
    ��  ��  ����Clipping.cpp
    ��  ��
    ��  ����MotionCapture
    ��  ��  ����MotionCapture.vcxproj
    ��  ��  ����MotionCapture.cpp
    ��  ��
    ��  ����FaceTrackingSDK
    ��      ����FaceTrackingSDK.vcxproj
    ��      ����FaceTrackingSDK.cpp
    ��
    ��  // �v���p�e�B�V�[�g
    ����KinectBook.props
    ��
    ��  // ���C�Z���X
    ����License.txt
    ��
    ��  // ����
    ����ReadMe.txt


���T���v���v���O�����ɂ���
�{�T���v���v���O�����͈ȉ��̊��Ńr���h�ł���悤�ɐݒ肵�Ă��܂��B
���ɂ���Ă͓K�X�ݒ�����������Ă��������B

    Visual C++ 2010 Express
    Kinect for Windows SDK v1.6
    Kinect for Windows Developer Toolkit v1.6
    OpenCV 2.4.2
    DirectX SDK (June 2010)


��Kinect�ɂ���
Kinect for Xbox360�ł�Near Mode�͓��삵�Ȃ����߁A�\�[�X�R�[�h�̊Y���ӏ����R�����g�A�E�g���Ă��������B

    /*
    // Near Mode�̐ݒ�
    hResult = pSensor->NuiImageStreamSetImageFrameFlags( hDepthPlayerHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE );
    if( FAILED( hResult ) ){
        std::cerr << "Error : NuiImageStreamSetImageFrameFlags" << std::endl;
        return -1;
    }
    */

    NUI_IMAGE_DEPTH_MAXIMUM/*_NEAR_MODE*/

�܂��AKinect����肭���삵�Ȃ��ꍇ��Kinect for Windows Developer Toolkit��Kinect Explorer�ŊȒP�ɏ󋵂��m�F���邱�Ƃ��ł��܂��B
(�����̓v���O��������HRESULT�^�̃G���[�R�[�h����ł��m�F���邱�Ƃ��ł��܂��B)

    ��FNotPowered �� �d�����ʂ��Ă��Ȃ��\��������܂��B
                      USB/�d���P�[�u���̐ڑ����m�F���Ă��������B

USB�R���g���[���[�Ƃ̑����������ł���ꍇ������܂��B
�ʂ�USB�R���g���[���[�Ő��䂳��Ă���USB�|�[�g�ɐڑ��������Ə�肭���삷�邱�Ƃ�����܂��B


���v���p�e�B�[�V�[�g�ɂ���
�t���̃v���p�e�B�V�[�gKinectBook.props�ɂ͈ȉ��̐ݒ肪�L�q����Ă��܂��B
�쐬�����v���W�F�N�g�̐ݒ�ɖ��ɗ��Ǝv���܂��B
�Ȃ��A�{�T���v���v���O�����͕t���̃v���p�e�B�[�V�[�g�Ɉˑ����Ă��܂���B

    Kinect for Windows SDK v1.6
    Kinect for Windows Developer Toolkit v1.6
    OpenCV 2.4.2


��OpenCV�ɂ���
OpenCV�̓o�[�W�����ɂ���ă��C�u���������قȂ�܂��B
����͈ȉ��̖����K���Ń��C�u�������������Ă��܂��B

Debug�p�̃��C�u�����t�@�C���ɂ͍Ō�Ɂud�v�����܂��B

    Debug   �Fopencv_������������d.lib
    Release �Fopencv_������������.lib

�������̓o�[�W�����ԍ��ł��B

    ��FOpenCV 2.4.2 �� opencv_������242.lib

�������̓��W���[�����ł��B
���p����@�\�ɂ��킹�ă��C�u�����������N���܂��B
�悭�g���郂�W���[���͈ȉ��̂Ƃ���ł��B

    core    �F��b
    highgui �F���[�U�[�C���^�[�t�F�[�X
    imgproc �F�摜����

    ��Fcore �� opencv_core������.lib

�C���N���[�h����w�b�_�t�@�C����<opencv/opencv.hpp>�ł��B
�ꕔ�������قڂ��ׂĂ�OpenCV�֘A�̃w�b�_�t�@�C�����C���N���[�h����܂��B

    #include <opencv/opencv.hpp>

OpenCV 2.4.3�ɂ�Visual Studio 2012(Visual Studio 11)�����̃o�C�i������������Ă��܂���D
OpenCV 2.4.3���g�p����ꍇ��CMake���g����Visual Studio 2012(Visual Studio 11�܂���Visual Studio 11 Win64)�̃\�����[�V�������쐬���C�r���h���Ă��������D

    OpenCV v2.4.3 documentation > OpenCV Tutorials > Introduction to OpenCV > Installation in Windows > Building the library
    http://docs.opencv.org/trunk/doc/tutorials/introduction/windows_install/windows_install.html#building-the-library


�����ϐ��ɂ���
Kinect for Windows SDK�����Kinect for Windows Developer Toolkit�ADirectX SDK (June 2010)���C���X�g�[������ƈȉ��̊��ϐ����ݒ肳��܂��B
�{�T���v���v���O�����̊e��ݒ�͊��ϐ��Ɉˑ����Ă��܂��B
����̃o�[�W�����A�b�v�ɔ������ϐ��̕ϐ�����l���ω�����ꍇ������̂œK�X�ݒ�����������Ă��������B

    �C���X�g�[���Ŏ����Őݒ肳�����ϐ�
        �ϐ���          �l
        KINECTSDK10_DIR C:\Program Files\Microsoft SDKs\Kinect\v1.6\
        FTSDK_DIR       C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.6.0\
        DXSDK_DIR       C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\

    �蓮�ŐV�K�ɍ쐬������ϐ�
        �ϐ���          �l
        OPENCV_DIR      C:\Program Files\opencv\build\

    �蓮�Œl��ǉ�������ϐ�(�l�̊Ԃ́u;�v�ŋ�؂�܂�)
        �ϐ���          �l(Win32)
        Path            ;%FTSDK_DIR%Redist\x86;%OPENCV_DIR%x86\vc10\bin;%OPENCV_DIR%common\tbb\ia32\vc10
                        �l(x64)
                        ;%FTSDK_DIR%Redist\amd64;%OPENCV_DIR%x64\vc10\bin;%OPENCV_DIR%common\tbb\intel64\vc10

�܂��A���ϐ��ő��̕ϐ����Q�Ƃ���ꍇ��%������%�ƋL�q���܂��B
Visual C++�Ŋ��ϐ����Q�Ƃ���ꍇ��$(������)�ƋL�q���܂��B
���ꂼ��ϐ��̒l���W�J����ĔF������܂��B

    ��FFace Tracking SDK��Win32���s�t�@�C��(*.dll)�̃f�B���N�g�����w�肷��ꍇ
            ���ϐ�   �F %FTSDK_DIR%Redist\x86 �� C:\Program Files\Microsoft SDKs\Kinect\Developer Toolkit v1.6.0\Redist\x86

        Kinect for Windows SDK�̃C���N���[�h �f�B���N�g�����w�肷��ꍇ
            Visual C++ �F $(KINECTSDK10_DIR)inc �� C:\Program Files\Microsoft SDKs\Kinect\v1.6\inc

���ϐ��̐ݒ���ԈႦ���OS���N�����Ȃ��Ȃ�\��������̂Œ��ӂ��Ă��������B
�܂��A���ϐ���ύX�����ꍇ��Visual C++���ċN�����ĕύX�𔽉f���Ă��������B


��x64�v���b�g�t�H�[���ł̃r���h�ɂ���
Visual C++ 2010 Express�͕W���ł�x64�v���b�g�t�H�[���Ńr���h�ł��܂���B
(Express�ȊO��Visual Studio���i�ł͕W����x64�v���b�g�t�H�[���Ńr���h�ł��܂��B)
Visual C++ 2010 Express��x64�v���b�g�t�H�[���Ńr���h����ɂ́A
Windows SDK 7.1���C���X�g�[�����āA�v���W�F�N�g�̃v���p�e�B�y�[�W��[�\���v���p�e�B]>[�S��]�́u�v���b�g�t�H�[���c�[���Z�b�g�v��
�uv100�v����uWindows7.1SDK�v�ɕύX���邱�ƂŃr���h�ł���悤�ɂȂ�܂��B

    Microsoft Windows SDK for Windows 7 and .NET Framework 4 (Windows SDK 7.1)
    http://www.microsoft.com/en-us/download/details.aspx?id=8279

x64�v���b�g�t�H�[���Ńr���h����ɂ́AVisual C++�̃��j���[����[�r���h]>[�\���}�l�[�W���[]��I�����A�u�\���}�l�[�W���[�v�E�B���h�E��\�����܂��B
[�A�N�e�B�u �\�����[�V���� �v���b�g�t�H�[��]����[<�V�K�쐬...>]��I�����A�u�V�����\�����[�V���� �v���b�g�t�H�[���v�E�B���h�E��\�����܂��B
[�V�����v���b�g�t�H�[������͂܂��͑I�����Ă�������]�̃h���b�v�_�E�����X�g�������A�u64�r�b�g �v���b�g�t�H�[���v��I�����܂��B

�v���W�F�N�g�̊e��ݒ�(���C�u���� �f�B���N�g���Ȃ�)��x64�̐ݒ�����Ă��������B
�ڂ����͈ȉ��̃y�[�W���Q�Ƃ��Ă��������B

    MSDN���C�u���� > Visual C++ > Visual C++�ɂ��64�r�b�g�v���O���~���O > ���@ : Visual C++ �v���W�F�N�g�� 64 �r�b�g �v���b�g�t�H�[���p�ɐݒ肷��
    http://msdn.microsoft.com/ja-jp/library/9yb4317s(v=vs.100).aspx

�Ȃ��AVisual Studio Express 2012 for Windows Desktop�ł͕W����x64�v���b�g�t�H�[���Ńr���h�ł��܂��B


���X�^�[�g�A�b�v �v���W�F�N�g�ɂ���
�X�^�[�g�A�b�v �v���W�F�N�g�͕W���ł́u�V���O�� �X�^�[�g�A�b�v �v���W�F�N�g�v�ɂȂ��Ă��܂��B
���s����v���W�F�N�g��ύX����ɂ́A�\�����[�V�����̃R���e�L�X�g���j���[����[�v���p�e�B]��I���A
[���L�v���p�e�B]>[�X�^�[�g�A�b�v�v���W�F�N�g]�́u�V���O�� �X�^�[�g�A�b�v �v���W�F�N�g�v�̃h���b�v�_�E�����j���[������s�������v���W�F�N�g��I�����Đݒ肵�Ă��������B
�܂��A[���݂̑I��]�Ƀ`�F�b�N�{�b�N�X�����Đݒ肷��ƁA���ݑI������Ă���v���W�F�N�g���X�^�[�g�A�b�v �v���W�F�N�g�ɂȂ�̂ŕ֗��ł��B


���������x�ɂ���
�T���v���v���O�����𓮂����ꍇ�́uRelease�v�\���Ńr���h���Ă��������B
�R���p�C���̍œK�����L���ɂȂ�܂��B


������m�F
�{�T���v���v���O�����͈ȉ��̊��œ�����m�F���܂����B
�{�T���v���v���O�����͂��ׂĂ̊��ɂ��ē����ۏ؂�����̂ł͂���܂���B

    OS      �FWindows 7 SP1
    Sensor  �FKinect for Windows
    IDE     �FVisual C++ 2010 Express SP1
    Library �FKinect for Windows SDK v1.6
              Kinect for Developer Toolkit v1.6
              OpenCV 2.4.2
              DirectX SDK (June 2010)


�����C�Z���X�ɂ���
�{�T���v���v���O�����̃��C�Z���X��MIT License�Ƃ��܂��B
���C�Z���X����License.txt���Q�Ƃ��Ă��������B


���Ɛӎ���
�{�T���v���v���O�����͎g�p�҂̐ӔC�ɂ����ė��p���Ă��������B
���̃T���v���v���O�����ɂ���Ĕ������������Ȃ��Q�E���Q�Ȃǂɑ΂��āA���쌠�҂͈�ؐӔC�𕉂�Ȃ����̂Ƃ��܂��B
�܂��A�{�T���v���v���O�����͗\���Ȃ��ύX��폜����ꍇ������܂��B


���ύX����
2012/11/19 �T���v���v���O�������쐬���܂����B
